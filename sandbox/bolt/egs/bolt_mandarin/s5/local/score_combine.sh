#!/bin/bash

# Copyright 2012-2013  Arnab Ghoshal
#                      Johns Hopkins University (authors: Daniel Povey, Sanjeev Khudanpur)
#                2014  Guoguo Chen

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


# Script for system combination using minimum Bayes risk decoding.
# This calls lattice-combine to create a union of lattices that have been 
# normalized by removing the total forward cost from them. The resulting lattice
# is used as input to lattice-mbr-decode. This should not be put in steps/ or 
# utils/ since the scores on the combined lattice must not be scaled.

# begin configuration section.
cmd=run.pl
beam=5 # prune the lattices prior to MBR decoding, for speed.
stage=0
cer=0
decode_mbr=true
lat_weights=
word_ins_penalty=0.0
min_lmwt=7
max_lmwt=17
parallel_opts="-pe smp 3"
skip_scoring=false
ctm_name=
#end configuration section.

[ -f ./path.sh ] && . ./path.sh
. parse_options.sh || exit 1;

if [ $# -lt 5 ]; then
  echo "Usage: $0 [options] <data-dir> <graph-dir|lang-dir> \\"
  echo "          <decode-dir1>[:lmwt-bias] <decode-dir2>[:lmwt-bias] \\"
  echo "          [<decode-dir3>[:lmwt-bias] ... ] <out-dir>"
  echo " e.g.: $0 data/test data/lang exp/tri1/decode \\"
  echo "          exp/tri2/decode:18 exp/tri3/decode:13 exp/combine"
  echo "Options:"
  echo "  --cmd (run.pl|queue.pl...)  # specify how to run the sub-processes."
  echo "  --min-lmwt INT              # minumum LM-weight for lattice rescoring"
  echo "  --max-lmwt INT              # maximum LM-weight for lattice rescoring"
  echo "  --lat-weights STR           # colon-separated string of lattice"
  echo "                              # weights"
  echo "  --cmd (run.pl|queue.pl...)  # specify how to run the sub-processes."
  echo "  --stage (0|1|2)             # (createCTM | filterCTM | runSclite)."
  echo "  --parallel-opts <string>    # extra options to command for"
  echo "                              # combination stage, default '-pe smp 3'"
  echo "  --cer (0|1)                 # compute CER in addition to WER"
  exit 1;
fi

data=$1
lang=$2
dir=${@: -1}  # last argument to the script
shift 2;
decode_dirs=( $@ )  # read the remaining arguments into an array
unset decode_dirs[${#decode_dirs[@]}-1]  # 'pop' the last argument which is odir
num_sys=${#decode_dirs[@]}  # number of systems to combine

#Let the user to set the CTM file name 
#use the data-dir name in case the user doesn't care
if [ -z ${ctm_name} ] ; then
  ctm_name=`basename $data`
fi

for f in $lang/words.txt $lang/phones/word_boundary.int ; do
  [ ! -f $f ] && echo "$0: file $f does not exist" && exit 1;
done
if ! $skip_scoring ; then
  for f in  $data/stm; do
    [ ! -f $f ] && echo "$0: file $f does not exist" && exit 1;
  done
fi


mkdir -p $dir/log

for i in `seq 0 $[num_sys-1]`; do
  decode_dir=${decode_dirs[$i]}
  offset=`echo $decode_dir | cut -d: -s -f2` # add this to the lm-weight.
  decode_dir=`echo $decode_dir | cut -d: -f1`
  [ -z "$offset" ] && offset=0
  
  model=`dirname $decode_dir`/final.mdl  # model one level up from decode dir

  [ ! -f $decode_dir/lat.1.gz ] &&\
    echo "$0: expecting file $f to exist" && exit 1;

  # If the lattices were generated by other toolkit and were converted into
  # Kaldi format, we won't have the corresponding Kaldi model, and we can't do
  # lattice-align-words.
  if [ -f $model ]; then
    lats[$i]="ark:lattice-scale --inv-acoustic-scale=\$[$offset+LMWT]"
    lats[$i]+=" 'ark:gunzip -c $decode_dir/lat.*.gz|' ark:- |"
    lats[$i]+=" lattice-add-penalty --word-ins-penalty=$word_ins_penalty"
    lats[$i]+=" ark:- ark:- | lattice-prune --beam=$beam ark:- ark:- |"
    lats[$i]+=" lattice-align-words $lang/phones/word_boundary.int"
    lats[$i]+=" $model ark:- ark:- |"
  else
    lats[$i]="ark:lattice-scale --inv-acoustic-scale=\$[$offset+LMWT]"
    lats[$i]+=" 'ark:gunzip -c $decode_dir/lat.*.gz|' ark:- |"
    lats[$i]+=" lattice-add-penalty --word-ins-penalty=$word_ins_penalty"
    lats[$i]+=" ark:- ark:- | lattice-prune --beam=$beam ark:- ark:- |"
  fi
done

mkdir -p $dir/scoring/log

if [ -z "$lat_weights" ]; then
  lat_weights=1.0
  for i in `seq $[$num_sys-1]`; do lat_weights="$lat_weights:1.0"; done
fi

if [ $stage -le 0 ]; then  
  $cmd $parallel_opts LMWT=$min_lmwt:$max_lmwt $dir/log/combine_lats.LMWT.log \
    mkdir -p $dir/score_LMWT/ '&&' \
    lattice-combine --lat-weights=$lat_weights "${lats[@]}" ark:- \| \
    lattice-to-ctm-conf --decode-mbr=true ark:- - \| \
    utils/int2sym.pl -f 5 $lang/words.txt  \| \
    tee $dir/score_LMWT/${ctm_name}.utt.ctm \| \
    local/convert_ctm.pl $data/segments $data/reco2file_and_channel \
    '>' $dir/score_LMWT/${ctm_name}.ctm || exit 1;
fi


if [ $stage -le 1 ]; then
  # Remove some stuff we don't want to score, from the ctm.
  for lmwt in `seq $min_lmwt $max_lmwt`; do
    x=$dir/score_${lmwt}/${ctm_name}.ctm
    [ ! -f $x ] && echo "File $x does not exist! Exiting... " && exit 1
    cp $x $x.bkup1;
    cat $x.bkup1 | grep -v -E '\[NOISE|LAUGHTER|VOCALIZED-NOISE\]' | \
      grep -v -E '<UNK>|%HESITATION|\(\(\)\)' | \
      grep -v -E '<eps>' | \
      grep -v -E '<hes>' | \
      grep -v -E '<noise>' | \
      grep -v -E '<silence>' | \
      grep -v -E '<unk>' | \
      grep -v -E '<v-noise>' | \
      perl -e '@list = (); %list = ();
      while(<>) {
        chomp; 
        @col = split(" ", $_); 
        push(@list, $_);
        $key = "$col[0]" . " $col[1]"; 
        $list{$key} = 1;
      } 
      foreach(sort keys %list) {
        $key = $_;
        foreach(grep(/$key/, @list)) {
          print "$_\n";
        }
      }' > $x;
    cp $x $x.bkup2;
  done
fi

if ! $skip_scoring ; then
  if [ $stage -le 2 ]; then
    local/score_stm.sh --min-lmwt $min_lmwt \
      --max-lmwt $max_lmwt $data $lang $dir || exit 1
  fi
fi

exit 0
