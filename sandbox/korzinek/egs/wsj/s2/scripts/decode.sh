#!/bin/bash   
# Copyright 2010-2011 Microsoft Corporation

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

orig_args="$*"
. path.sh
# will set nj to #spkrs (if using queue) or 4 (if not), if
# not set by the user.
nj=
lang=
opts=
cmd=scripts/run.pl
for x in 1 2 3 4; do
  if [ $1 == "--num-jobs" ]; then
     nj=$2
     shift 2
  fi
  if [ $1 == "--opts" ]; then
     opts="$2"
     shift 2
  fi
  if [ $1 == "--cmd" ]; then
     cmd=$2
     shift 2
     [ -z "$cmd" ] && echo "Empty argument to --cmd option" && exit 1;
  fi  
  if [ $1 == "-l" ]; then
     lang=$2
     shift 2
     [ ! -f "$lang/phones_disambig.txt" -o ! -f "$lang/L_align.fst" ] && \
      echo "Invalid argument to -l option; expected $lang/phones_disambig.txt and $lang/L_align.fst to exist." \
      && exit 1;
  fi  
done


if [ $# -lt 4 ]; then
  echo "Usage: scripts/decode.sh [-l lang-dir] [--cmd scripts/queue.sh opts..] [--num-jobs n] <decode_script> <graph-dir> <data-dir> <decode-dir> [extra-args...]"
  echo "note: -l option only required if you want to score with sclite (since we need L_align.fst)"
  exit 1;
fi

script=$1
graphdir=$2
data=$3
dir=$4
# Make "dir" an absolute pathname.
dir=`perl -e '($dir,$pwd)= @ARGV; if($dir!~m:^/:) { $dir = "$pwd/$dir"; } print $dir; ' $dir ${PWD}`
mkdir -p $dir || exit 1
shift;shift;shift;shift;
# Remaining args will be supplied to decoding script.
extra_args=$* 

for file in $script $scp $data/utt2spk; do
  if [ ! -f "$file" ]; then
     echo "decode.sh: no such file $file"
     exit 1
  fi 
done

if [ ! -f $graphdir/HCLG.fst -a ! -f $graphdir/G.fst ]; then
  # Note: most scripts expect HCLG.fst in graphdir, but the
  # "*_fromlats.sh" script(s) require(s) a "lang" dir in that
  # position
  echo No such file: $graphdir/HCLG.fst or $graphdir/G.fst
  exit 1;
fi

if [ "$nj" == "" ]; then # Figure out num-jobs; user did not specify.
  cmd1=`echo $cmd | awk '{print $1;}'`
  if [ `basename $cmd1` == run.pl ]; then
    nj=4
  else # running on queue...
    nj=`scripts/utt2spk_to_spk2utt.pl $data/utt2spk | wc -l`
  fi
fi

echo "Decoding with num-jobs = $nj"
if [[ $nj -gt 1 || ! -d $data/split$nj || $data/split$nj -ot $data/feats.scp ]]; then
  scripts/split_data.sh $data $nj
fi

rm $dir/.error 2>/dev/null
for n in `scripts/get_splits.pl $nj`; do
  $cmd $dir/part$n.log \
    $script $opts -j $nj $n $graphdir $data $dir $extra_args || touch $dir/.error &
done

wait
[ -f $dir/.error ] && echo "Error in decoding script: command line was decode.sh $orig_args" && exit 1;

if ls $dir/lat.*.gz >&/dev/null; then
  if [ -n "$lang" ]; then # sclite scoring: $lang directory supplied only for this reason.
    [ ! -f $data/stm ] && \
     echo "Expected $data/stm to exist (-l option only for sclite scoring)" && exit 1;
    scripts/score_lats_ctm.sh $dir $lang $data || exit 1;
  else
    scripts/score_lats.sh $dir $graphdir/words.txt $data || exit 1;
  fi
elif ls $dir/*.txt >&/dev/null; then
  scripts/score_text.sh $dir $data || exit 1;
else
  echo "No output found in $dir, not scoring.";
  exit 1;
fi
