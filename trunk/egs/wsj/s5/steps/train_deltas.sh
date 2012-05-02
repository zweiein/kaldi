#!/bin/bash

# Copyright 2012  Daniel Povey
# Apache 2.0

# Begin configuration.
stage=-4 #  This allows restarting after partway, when something when wrong.
cmd=run.pl
scale_opts="--transition-scale=1.0 --acoustic-scale=0.1 --self-loop-scale=0.1"
realign_iters="10 20 30";
num_iters=35    # Number of iterations of training
max_iter_inc=25 # Last iter to increase #Gauss on.
beam=10
retry_beam=40
# End configuration.

[ -f path.sh ] && . ./path.sh;
. parse_options.sh || exit 1;

if [ $# != 6 ]; then
   echo "Usage: steps/train_deltas.sh <num-leaves> <tot-gauss> <data-dir> <lang-dir> <alignment-dir> <exp-dir>"
   echo "e.g.: steps/train_deltas.sh 2000 10000 data/train_si84_half data/lang exp/mono_ali exp/tri1"
   echo "main options (for others, see top of script file)"
   echo "  --cmd (utils/run.pl|utils/queue.pl <queue opts>) # how to run jobs."
   echo "  --config <config-file>                           # config containing options"
   echo "  --stage <stage>                                  # stage to do partial re-run from."
   exit 1;
fi

numleaves=$1
totgauss=$2
data=$3
lang=$4
alidir=$5
dir=$6

for f in $alidir/final.mdl $alidir/ali.1.gz $data/feats.scp $lang/phones.txt; do
  [ ! -f $f ] && echo "train_deltas.sh: no such file $f" && exit 1;
done

numgauss=$numleaves
incgauss=$[($totgauss-$numgauss)/$max_iter_inc] # per-iter increment for #Gauss
oov=`cat $lang/oov.int` || exit 1;
ciphonelist=`cat $lang/phones/context_indep.csl` || exit 1;
nj=`cat $alidir/num_jobs` || exit 1;
mkdir -p $dir/log
echo $nj > $dir/num_jobs

sdata=$data/split$nj;
[[ -d $sdata && $data/feats.scp -ot $sdata ]] || split_data.sh $data $nj || exit 1;

feats="ark,s,cs:apply-cmvn --norm-vars=false --utt2spk=ark:$sdata/JOB/utt2spk scp:$sdata/JOB/cmvn.scp scp:$sdata/JOB/feats.scp ark:- | add-deltas ark:- ark:- |"

rm $dir/.error 2>/dev/null

if [ $stage -le -3 ]; then
  echo "Accumulating tree stats"
  $cmd JOB=1:$nj $dir/log/acc_tree.JOB.log \
    acc-tree-stats  --ci-phones=$ciphonelist $alidir/final.mdl "$feats" \
     "ark:gunzip -c $alidir/ali.JOB.gz|" $dir/JOB.treeacc || exit 1;
  sum-tree-stats $dir/treeacc $dir/*.treeacc 2>$dir/log/sum_tree_acc.log || exit 1;
  rm $dir/*.treeacc
fi

if [ $stage -le -2 ]; then
  echo "Getting questions for tree-building, via clustering"
  # preparing questions, roots file...
  cluster-phones $dir/treeacc $lang/phones/sets.int $dir/questions.int 2> $dir/log/questions.log || exit 1;
  cat $lang/phones/extra_questions.int >> $dir/questions.int
  compile-questions $lang/topo $dir/questions.int $dir/questions.qst 2>$dir/log/compile_questions.log || exit 1;

  echo "Building the tree"
  $cmd $dir/log/build_tree.log \
    build-tree --verbose=1 --max-leaves=$numleaves \
     $dir/treeacc $lang/phones/roots.int \
     $dir/questions.qst $lang/topo $dir/tree || exit 1;

  gmm-init-model  --write-occs=$dir/1.occs  \
    $dir/tree $dir/treeacc $lang/topo $dir/1.mdl 2> $dir/log/init_model.log || exit 1;
  grep 'no stats' $dir/log/init_model.log && echo "This is a bad warning.";

  gmm-mixup --mix-up=$numgauss $dir/1.mdl $dir/1.occs $dir/1.mdl 2>$dir/log/mixup.log || exit 1;
  rm $dir/treeacc
fi

if [ $stage -le -1 ]; then
  # Convert the alignments.
  echo "Converting alignments from $alidir to use current tree"
  $cmd JOB=1:$nj $dir/log/convert.JOB.log \
    convert-ali $alidir/final.mdl $dir/1.mdl $dir/tree \
     "ark:gunzip -c $alidir/ali.JOB.gz|" "ark:|gzip -c >$dir/ali.JOB.gz" || exit 1;
fi

if [ $stage -le 0 ]; then
  echo "Compiling graphs of transcripts"
  $cmd JOB=1:$nj $dir/log/compile_graphs.JOB.log \
    compile-train-graphs $dir/tree $dir/1.mdl  $lang/L.fst  \
     "ark:utils/sym2int.pl --map-oov $oov -f 2- $lang/words.txt < $data/split$nj/JOB/text |" \
      "ark:|gzip -c >$dir/fsts.JOB.gz" || exit 1;
fi

x=1
while [ $x -lt $num_iters ]; do
  echo Training pass $x
  if [ $stage -le $x ]; then
    if echo $realign_iters | grep -w $x >/dev/null; then
      echo Aligning data
      $cmd JOB=1:$nj $dir/log/align.$x.JOB.log \
        gmm-align-compiled $scale_opts --beam=$beam --retry-beam=$retry_beam $dir/$x.mdl \
         "ark:gunzip -c $dir/fsts.JOB.gz|" "$feats" \
         "ark:|gzip -c >$dir/ali.JOB.gz" || exit 1;
    fi
    $cmd JOB=1:$nj $dir/log/acc.$x.JOB.log \
      gmm-acc-stats-ali  $dir/$x.mdl "$feats" \
       "ark,s,cs:gunzip -c $dir/ali.JOB.gz|" $dir/$x.JOB.acc || exit 1;
    $cmd $dir/log/update.$x.log \
      gmm-est --mix-up=$numgauss --write-occs=$dir/$[$x+1].occs $dir/$x.mdl \
       "gmm-sum-accs - $dir/$x.*.acc |" $dir/$[$x+1].mdl || exit 1;
    rm $dir/$x.mdl $dir/$x.*.acc
    rm $dir/$x.occs
  fi
  [ $x -le $max_iter_inc ] && numgauss=$[$numgauss+$incgauss];
  x=$[$x+1];
done

rm $dir/final.mdl 2>/dev/null
ln -s $x.mdl $dir/final.mdl
ln -s $x.occs $dir/final.occs

# Summarize warning messages...
for x in $dir/log/*.log; do 
  n=`grep WARNING $x | wc -l`; [ $n -ne 0 ] && echo $n warnings in $x;
done

echo Done training system with delta+delta-delta features in $dir
