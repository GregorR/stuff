#!/bin/bash -x
if [ ! "$3" ]
then
    echo 'Use: threepass.sh <input file> <identification file> <output file> [video bitrate] [encoding options] [playback options]'
    exit 1
fi

IN="$1"
IDENT="$2"
OUT="$3"
OUT_NSP=`echo $OUT | sed 's/ /_/g'`
VBR="$4"
if [ "$VBR" = "" ] ; then VBR=800 ; fi
ENCOPTS="$5"
PLAYOPTS="$6"


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
    faac $OUT_NSP.wav.$i -o $OUT_NSP.$i.aac < /dev/null &

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
        faac $OUT_NSP.wav.stereo.$i -o $OUT_NSP.$i.stereo.aac < /dev/null &
    fi
done

# Put all the stereo tracks, then any non-stereo tracks
for i in $AUDIO_TRACKS
do
    NCH=`nch "$IDENT" $i`
    ALANG=`aid_lang "$IDENT" $i`
    if [ "$NCH" = "2" ]
    then
        AUDIO_MKV="$AUDIO_MKV --language -1:$ALANG $OUT_NSP.$i.aac"
        AUDIO_MP4="$AUDIO_MP4 $OUT_NSP.$i.aac#audio:lang=$ALANG"

    elif [ "$CONVERT_SURROUND" = "yes" ]
    then
        AUDIO_MKV="$AUDIO_MKV --language -1:$ALANG $OUT_NSP.$i.stereo.aac"
        AUDIO_MP4="$AUDIO_MP4 $OUT_NSP.$i.aac#audio:lang=$ALANG"
    fi
done
for i in $AUDIO_TRACKS
do
    NCH=`nch "$IDENT" $i`
    ALANG=`aid_lang "$IDENT" $i`
    if [ "$NCH" != "2" ]
    then
        AUDIO_MKV="$AUDIO_MKV --language -1:$ALANG $OUT_NSP.$i.aac"
        AUDIO_MP4="$AUDIO_MP4 $OUT_NSP.$i.aac#audio:lang=$ALANG"
    fi
done


###############################################################################
# SUBTITLES
###############################################################################

# Now pull out any subtitles
SUB_TRACKS="`midentify $IDENT | grep '^ID_SUBTITLE_ID=' | sed 's/ID_SUBTITLE_ID=//'`"
SUB_IDXS=""
for i in $SUB_TRACKS
do
    mencoder \
        $IDENT \
        -ovc copy -nosound \
        -sid $i -vobsubout $OUT_NSP.$i \
        -o /dev/null < /dev/null &
    SUB_IDXS="$SUB_IDXS $OUT_NSP.$i.idx"
done


###############################################################################
# VIDEO
###############################################################################

# Pass 1
mencoder \
   -ovc x264 -oac copy \
   -x264encopts bitrate=$VBR:pass=1:threads=auto \
   -passlogfile "$OUT".log \
   $ENCOPTS \
   $IN \
   -o /dev/null

# Pass 2
mencoder \
   -ovc x264 -oac copy \
   -x264encopts bitrate=$VBR:pass=3:threads=auto \
   -passlogfile "$OUT".log \
   $ENCOPTS \
   $IN \
   -o /dev/null

# Pass 3
mencoder \
   -ovc x264 -oac copy \
   -x264encopts bitrate=$VBR:pass=3:threads=auto \
   -passlogfile "$OUT".log \
   $ENCOPTS \
   $IN \
   -o "$OUT".vid.avi

# Make sure audio is done
wait

# Mux it
mkvmerge \
    -A "$OUT".vid.avi $AUDIO_MKV $SUB_IDXS \
    -o "$OUT".mkv
ffmpeg -i "$OUT".vid.avi -vcodec copy -an "$OUT".vid.h264
MP4Box \
    "$OUT.vid.h264#video" $AUDIO_MP4 \
    "$OUT".mp4

# Clean up
for i in $AUDIO_TRACKS
do
    rm -f $OUT_NSP.wav.$i $OUT_NSP.$i.aac $OUT_NSP.wav.stereo.$i $OUT_NSP.$i.stereo.aac
done
for i in $SUB_TRACKS
do
    rm -f $OUT_NSP.$i.idx $OUT_NSP.$i.sub
done
rm -f "$OUT".log "$OUT".log.temp "$OUT".vid.avi "$OUT".vid.h264
