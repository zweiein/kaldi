#!/bin/bash
# Copyright 2010-2011 Microsoft Corporation  Arnab Ghoshal

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
# WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
# MERCHANTABLITY OR NON-INFRINGEMENT.
# See the Apache 2 License for the specific language governing permissions and
# limitations under the License.

# To be run from ..
# Triphone model training, with LDA + MLLT.
# LDA means: on each frame we splice 9 adjacent frames of the raw
# MFCC features together to get 9x13 = 117-dimensional feature vectors.
# Then we do LDA, where the classes correspond to the "pdf-ids" 
# (i.e. the distinct pdf's of the baseline system, typically several
# thousand, corresponding to the leaves of the phonetic-context tree).
# The stats for this consist of one or two symmetric 117x117 matrices,
# plus 117-dimensional means for each of the several thousand classes.
# We actually have some randomized sub-sampling going on too (see "randprune" 
# parameter), to speed it up (the 117-dimensional outer products are
# a bit slow).  
# The LDA gives us 40x117 matrix, and corresponding 40-dimensional
# features.  These features will contain most of the inter-class variation
# in the original features, and will be globally decorrelated.

# Then we do 
# MLLT (Maximum Likelihood Linear Transform; search for Gopinath), 
# also known as STC (semi-tied covariance-- search for Gales; our 
# version is the "global" version)... probably the best reference is
# M. Gales, "Maximum likelihood linear transformations for HMM-based speech recognition",
# Computer Speech and Language.
# The idea here is to find a square matrix-valued feature transformation
# that will maximize the likelihood of the data given the 
# mixture-of-diagonal-Gaussian models [after appropriately taking into
# account the Jacobian of the transformation matrix].  We just multiply
# this on the left of the original LDA matrix, so at the end of this whole procedure
# we have just one matrix.
# The variable "mllt_iters" specifies on which iterations of overall training
# we do an iteration of MLLT matrix estimation, and the variable
# "realign_iters" specifies on which iterations we re-do the Viterbi aligments.
# [All Kaldi training is based on Viterbi state-level aligments].

nj=4
cmd=scripts/run.pl

while [ 1 ]; do
  case $1 in
    --num-jobs)
      shift; nj=$1; shift;
      ;;
    --cmd)
      shift; cmd=$1; shift;
      ;;
    --scale-opts)
      shift; scale_opts=$1; shift;
      ;;
    *)
      break;
      ;;
  esac
done

if [ $# != 8 ]; then
   echo "Usage: steps/train_lda_mllt.sh <num-leaves> <tot-gauss> <data-dir-bn> <data-dir-mfcc> <lang-dir> <ali-dir> <transf2-dir> <exp-dir>"
   echo " e.g.: steps/train_lda_mllt.sh 2500 15000 data/train_si84 data/lang data_mfcc exp/tri2b_ali_si84 transf2_dir exp/tandem"
   exit 1;
fi

if [ -f path.sh ]; then . path.sh; fi

numleaves=$1
totgauss=$2
data=$3
mfcc=$4
lang=$5
alidir=$6
transf2dir=$7
dir=$8

#TODO DEFINE 2 STREAMS SEPARATELY!

if [ ! -f $alidir/final.mdl ]; then
  echo "Error: alignment dir $alidir does not contain final.mdl"
  exit 1;
fi

if [ ! -d $transf2dir ]; then
  echo "transf2dir : $transf2dir does not exist!";
  exit 1;
fi


echo scale_opts: ${scale_opts:="--transition-scale=1.0 --acoustic-scale=0.1 --self-loop-scale=0.1"}
realign_iters="10 20 30";
oov_sym=`cat $lang/oov.txt`
silphonelist=`cat $lang/silphones.csl`
numiters=35    # Number of iterations of training
maxiterinc=25 # Last iter to increase #Gauss on.
#numgauss=$numleaves
numgauss=$totgauss
incgauss=$[($totgauss-$numgauss)/$maxiterinc] # per-iter increment for #Gauss
randprune=4.0 # This is approximately the ratio by which we will speed up the
              # LDA and MLLT calculations via randomized pruning.

mkdir -p $dir/log

if [ ! -d $data/split$nj -o $data/split$nj -ot $data/feats.scp ]; then
  scripts/split_data.sh $data $nj
fi

if [ ! -d $mfcc/split$nj -o $mfcc/split$nj -ot $mfcc/feats.scp ]; then
  scripts/split_data.sh $mfcc $nj
fi

rm $dir/.error 2>/dev/null

# Copy the final transforms of individual streams
[ ! -f $alidir/final.mat ] && echo "Feature transform for stream-1 '$alidir/final.mat' does not exist!" && exit 1;
[ ! -f $transf2dir/final.mat ] && echo "Feature transform for stream-2 '$transf2dir/final.mat' does not exist!" && exit 1;
cp $alidir/final.mat $dir/final_s1.mat
cp $transf2dir/final.mat $dir/final_s2.mat


echo "Computing CMVN stats"
for n in `get_splits.pl $nj`; do
  compute-cmvn-stats --spk2utt=ark:$data/split$nj/$n/spk2utt scp:$data/split$nj/$n/feats.scp \
      ark:$dir/$n.cmvn 2>$dir/cmvn$n.log || touch $dir/.error &
done
wait;
[ -f $dir/.error ] && echo error computing CMVN stats && exit 1;




# featspart[n] gets overwritten later in the script.
# WITH THE TRANSFORM!
for n in `get_splits.pl $nj`; do
  splicedfeatspart[$n]="ark:apply-cmvn --norm-vars=false --utt2spk=ark:$data/split$nj/$n/utt2spk ark:$dir/$n.cmvn scp:$data/split$nj/$n/feats.scp ark:- |"
  featspart[$n]="${splicedfeatspart[$n]} transform-feats $dir/final_s2.mat ark:- ark:- |"
done

# mfcc features with LDA + MLLT for single pass retraining 
# WITH THE TRANSFORM!
for n in `get_splits.pl $nj`; do
  mfccpart[$n]="ark:apply-cmvn --norm-vars=false --utt2spk=ark:$mfcc/split$nj/$n/utt2spk ark:$alidir/$n.cmvn scp:$mfcc/split$nj/$n/feats.scp ark:- | splice-feats ark:- ark:- | transform-feats $dir/final_s1.mat ark:- ark:- |"
done


# define the nerged features
for n in `get_splits.pl $nj`; do
  twofeatspart[$n]="ark:append-feats '${mfccpart[$n]}' '${featspart[$n]}' ark:- |";
done


# Copy the tree from previous model...
cp $alidir/tree $dir/tree

echo "Accumulating Single Pass Retraining statistics"
# Single pass retraining, stats accumulation
for n in `get_splits.pl $nj`; do 
  $cmd $dir/_acc_stats_twofeats$n.log \
   gunzip -c $alidir/${n}.ali.gz \| \
   ali-to-post ark:- ark:- \| \
   gmm-acc-stats-twofeats $alidir/final.mdl "${mfccpart[$n]}" "${twofeatspart[$n]}" ark:- $dir/0.$n.acc \
   2>$dir/acc_stats_twofeats$n.log || touch $dir/.error;
done
wait;
[ -f $dir/.error ] && echo "Error accumulating Single Pass Retraining statistics" && exit 1;

# Update model.
echo "Updating model to new feature space"
x=0
$cmd $dir/log/update.$x.log \
 gmm-est --write-occs=$dir/$[$x+1].occs $alidir/final.mdl \
 "gmm-sum-accs - $dir/$x.*.acc |" $dir/$[$x+1].mdl || exit 1;

# Convert alignments in $alidir, to use as initial alignments.
# This assumes that $alidir was split in $nj pieces, just like the
# current dir.

echo "Converting old alignments"
for n in `get_splits.pl $nj`; do # do this locally: it's very fast.
  convert-ali $alidir/final.mdl $dir/1.mdl $dir/tree \
   "ark:gunzip -c $alidir/$n.ali.gz|" "ark:|gzip -c >$dir/$n.ali.gz" \
    2>$dir/log/convert$n.log || exit 1;
done

# Make training graphs (this is split in $nj parts).
echo "Compiling training graphs"
rm $dir/.error 2>/dev/null
for n in `get_splits.pl $nj`; do
  $cmd $dir/log/compile_graphs$n.log \
    compile-train-graphs $dir/tree $dir/1.mdl  $lang/L.fst  \
      "ark:scripts/sym2int.pl --map-oov \"$oov_sym\" --ignore-first-field $lang/words.txt < $data/split$nj/$n/text |" \
      "ark:|gzip -c >$dir/$n.fsts.gz" || touch $dir/.error &
done
wait;
[ -f $dir/.error ] && echo "Error compiling training graphs" && exit 1;

x=1
while [ $x -lt $numiters ]; do
   echo Pass $x
   if echo $realign_iters | grep -w $x >/dev/null; then
     echo "Aligning data"
     for n in `get_splits.pl $nj`; do
       $cmd $dir/log/align.$x.$n.log \
         gmm-align-compiled $scale_opts --beam=10 --retry-beam=40 $dir/$x.mdl \
           "ark:gunzip -c $dir/$n.fsts.gz|" "${twofeatspart[$n]}" \
           "ark:|gzip -c >$dir/$n.ali.gz" || touch $dir/.error &
     done
     wait;
     [ -f $dir/.error ] && echo "Error aligning data on iteration $x" && exit 1;
   fi
   ## The main accumulation phase.. ##
   for n in `get_splits.pl $nj`; do 
     $cmd $dir/log/acc.$x.$n.log \
       gmm-acc-stats-ali  $dir/$x.mdl "${twofeatspart[$n]}" \
         "ark,s,cs:gunzip -c $dir/$n.ali.gz|" $dir/$x.$n.acc || touch $dir/.error &
   done
   wait;
   [ -f $dir/.error ] && echo "Error accumulating stats on iteration $x" && exit 1;
   $cmd $dir/log/update.$x.log \
     gmm-est --write-occs=$dir/$[$x+1].occs --mix-up=$numgauss $dir/$x.mdl \
       "gmm-sum-accs - $dir/$x.*.acc |" $dir/$[$x+1].mdl || exit 1;
   rm $dir/$x.mdl $dir/$x.*.acc
   rm $dir/$x.occs 
   if [[ $x -le $maxiterinc ]]; then 
      numgauss=$[$numgauss+$incgauss];
   fi
   x=$[$x+1];
done

( cd $dir; rm final.mdl 2>/dev/null; ln -s $x.mdl final.mdl; ln -s $x.occs final.occs; )

echo Done
