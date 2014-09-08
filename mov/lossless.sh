#!/bin/bash -x
if [ ! "$3" ]
then
    echo 'Use: lossless.sh <input file> <identification file> <output file> [encoding options] [playback options]'
    exit 1
fi

IN="$1"
IDENT="$2"
OUT="$3"
OUT_NSP=`echo $OUT | sed 's/ /_/g'`
ENCOPTS="$4"
PLAYOPTS="$5"

REQUIRED="mplayer mencoder flac mkvmerge MP4Box"
for req in $REQUIRED
do
    $req --help >& /dev/null
    if [ "$?" = "127" ]
    then
        echo "$req not found."
        exit 1
    fi
done

midentify() {
    mplayer -vo null -ao null -frames 0 -identify "$@" 2>/dev/null |
        sed -ne '/^ID_/ {
                          s/[]()|&;<>`'"'"'\\!$" []/\\&/g;p
                        }'
}


###############################################################################
# AUDIO
###############################################################################

# Utility function: Number of channels in audio track $2 of $1
nch() {
    midentify $1 -aid $2 -channels 6 | grep '^ID_AUDIO_NCH=[^0]' | sed 's/ID_AUDIO_NCH=//'
}

# Get the language of audio track $2 of $1
aid_lang() {
    TEST_LANG="`midentify $1 | grep '^ID_AID_'$2'_LANG=' | head -n 1 | sed 's/ID_AID_[^_]*_LANG=//'`"
    if [ "$TEST_LANG" = "" ] ; then echo "und" ; else echo "$TEST_LANG" ; fi
}


# Get all the audio tracks
if [ ! "$AUDIO_TRACKS" ]
then
    AUDIO_TRACKS="`midentify $IDENT | grep '^ID_AUDIO_ID=' | sed 's/ID_AUDIO_ID=//' | sort | uniq`"
fi

# Count the surround-sound vs stereo
SURROUND=0
STEREO=0
for i in $AUDIO_TRACKS
do
    NCH=`nch "$IDENT" $i`
    if [ "$NCH" = "6" ]
    then
        SURROUND=$(( SURROUND + 1 ))
    else
        STEREO=$(( STEREO + 1 ))
    fi
done

# If there are more surround tracks than stereo tracks, assume they need to be converted
if [ $SURROUND -gt $STEREO ]
then
    CONVERT_SURROUND=yes
fi

# Extract the audio ...
for i in $AUDIO_TRACKS
do
    NCH=`nch "$IDENT" $i`
    mkfifo $OUT_NSP.wav.$i
    mplayer \
       -vc null -vo null \
       -aid $i -channels 6 -ao pcm:fast:file=$OUT_NSP.wav.$i \
       $PLAYOPTS \
       $IN \
       < /dev/null &
    flac $OUT_NSP.wav.$i -o $OUT_NSP.$i.flac < /dev/null &

    # And a stereo channel if necessary
    if [ "$NCH" = "6" -a "$CONVERT_SURROUND" = "yes" ]
    then
        mkfifo $OUT_NSP.wav.stereo.$i
        mplayer \
            -vc null -vo null \
            -aid $i -channels 2 -ao pcm:fast:file=$OUT_NSP.wav.stereo.$i \
            $PLAYOPTS \
            $IN \
            < /dev/null &
        flac $OUT_NSP.wav.stereo.$i -o $OUT_NSP.$i.stereo.flac < /dev/null &
    fi
done

# Put all the stereo tracks, then any non-stereo tracks
for i in $AUDIO_TRACKS
do
    NCH=`nch "$IDENT" $i`
    ALANG=`aid_lang "$IDENT" $i`
    if [ "$NCH" = "2" ]
    then
        AUDIO_MKV="$AUDIO_MKV --language -1:$ALANG $OUT_NSP.$i.flac"
        AUDIO_MP4="$AUDIO_MP4 -add $OUT_NSP.$i.flac#audio:lang=$ALANG"

    elif [ "$CONVERT_SURROUND" = "yes" ]
    then
        AUDIO_MKV="$AUDIO_MKV --language -1:$ALANG $OUT_NSP.$i.stereo.flac"
        AUDIO_MP4="$AUDIO_MP4 -add $OUT_NSP.$i.stereo.flac#audio:lang=$ALANG"
    fi
done
for i in $AUDIO_TRACKS
do
    NCH=`nch "$IDENT" $i`
    ALANG=`aid_lang "$IDENT" $i`
    if [ "$NCH" != "2" ]
    then
        AUDIO_MKV="$AUDIO_MKV --language -1:$ALANG $OUT_NSP.$i.flac"
        AUDIO_MP4="$AUDIO_MP4 -add $OUT_NSP.$i.flac#audio:lang=$ALANG"
    fi
done


###############################################################################
# SUBTITLES
###############################################################################

# Now pull out any subtitles
SUB_TRACKS="`midentify $IDENT | grep '^ID_SUBTITLE_ID=' | sed 's/ID_SUBTITLE_ID=//' | sort | uniq`"
SUB_MKV=""
SUB_MP4=""
for i in $SUB_TRACKS
do
    mencoder \
        $IDENT \
        -ovc copy -nosound \
        -sid $i -vobsubout $OUT_NSP.$i \
        -o /dev/null < /dev/null &
    SUB_MKV="$SUB_MKV $OUT_NSP.$i.idx"
    SUB_MP4="$SUB_MP4 -add $OUT_NSP.$i.idx" # FIXME: seems broken with MP4Box
done


###############################################################################
# VIDEO
###############################################################################

# Get the fps
fps() {
    midentify "$@" | grep '^ID_VIDEO_FPS=' | sed 's/ID_VIDEO_FPS=//'
}
FPS="`fps $IDENT`"

# Only one pass for lossless
mencoder \
   -ovc x264 -oac copy \
   -x264encopts qp=0:threads=auto \
   $ENCOPTS \
   $IN \
   -o "$OUT".vid.avi ||
mencoder \
   -ovc x264 -oac pcm \
   -x264encopts qp=0:threads=auto \
   $ENCOPTS \
   $IN \
   -o "$OUT".vid.avi

# Make sure audio is done
wait

# Mux it
ffmpeg -i "$OUT".vid.avi -vcodec copy -an "$OUT".vid.h264
mkvmerge \
    -A "$OUT".vid.h264 $AUDIO_MKV $SUB_MKV \
    -o "$OUT".mkv
#rm -f "$OUT".mp4
#MP4Box \
#    -inter 500 -isma \
#    -add "$OUT.vid.h264#video" -fps $FPS $AUDIO_MP4 $SUB_MP4 \
#    "$OUT".mp4

# Clean up
for i in $AUDIO_TRACKS
do
    rm -f $OUT_NSP.wav.$i $OUT_NSP.$i.flac $OUT_NSP.wav.stereo.$i $OUT_NSP.$i.stereo.flac
done
for i in $SUB_TRACKS
do
    rm -f $OUT_NSP.$i.idx $OUT_NSP.$i.sub
done
rm -f "$OUT".log "$OUT".log.temp "$OUT".vid.avi "$OUT".vid.h264
