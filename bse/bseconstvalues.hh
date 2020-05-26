// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_CONST_VALUES_H__
#define __BSE_CONST_VALUES_H__

#include <bse/bsedefs.hh>

/* --- frequencies --- */
/* kammer frequency, frequency of the note A of the standard piano
 * octave.slightly different values are used in different countries,
 * however, 440Hz is the official value for germany and much of europe.
 */
#define	BSE_KAMMER_FREQUENCY		(440.0)

/* maximum (audible) frequency represented in synthesis signals.
 * this value shouldn't be changed, to maintain frequency
 * representation in signals.
 */
#define	BSE_MAX_FREQUENCY		(24000.0)

/* minimum (needs to be > 0) and maximum audible frequencies supported
 * by oscillators (outer limits for user supplied frequency values)
 */
#define	BSE_MAX_OSC_FREQUENCY		(20000.0)
#define	BSE_MIN_OSC_FREQUENCY		(1.0 / BSE_MAX_OSC_FREQUENCY)

/* epsilon used to compare audible frequencies and check for equality */
#define BSE_FREQUENCY_EPSILON		(0.001)


/* --- transpose / fine tune --- */
#define	BSE_MIN_TRANSPOSE		(-72)
#define	BSE_MAX_TRANSPOSE		(+72)
#define BSE_TRANSPOSE_IS_VALID(n)	((n) >= BSE_MIN_TRANSPOSE && (n) <= BSE_MAX_TRANSPOSE)
/* fine tune in cents of a semitone */
#define	BSE_MIN_FINE_TUNE		(-BSE_MAX_FINE_TUNE)
#define	BSE_MAX_FINE_TUNE		(+100)
#define BSE_FINE_TUNE_IS_VALID(n)	((n) >= BSE_MIN_FINE_TUNE && (n) <= BSE_MAX_FINE_TUNE)


/* --- signal ranges --- */
/* min..max sample value: -1.0 .. 1.0
 * notes<->sample value: 0 .. 127 (BSE_VALUE_FROM_NOTE)
 * freq<->sample value: 0 .. 24000 (BSE_FREQ_FROM_VALUE)
 */
#define	BSE_FREQ_FROM_VALUE(value)	 (BSE_MAX_FREQUENCY * (value))
#define	BSE_VALUE_FROM_FREQ(freq)	 ((1.0 / BSE_MAX_FREQUENCY) * (freq))
#define	BSE_FREQUENCY_EPSILON		 (0.001)


/* --- volume --- */
#define	BSE_MIN_VOLUME_dB		(-144) /* theoretically: -96.3 */
#define	BSE_MAX_VOLUME_dB		(+24)


/* --- balance (left/right volume) --- */
#define	BSE_MIN_BALANCE         	(-100.0)
#define	BSE_MAX_BALANCE 		(+100.0)

/* --- bpm --- */
#define	BSE_MIN_BPM			(1)
#define	BSE_MAX_BPM			(1024)


/* --- time (unix seconds) --- */
#define BSE_MIN_TIME			(631148400)	/* 1990-01-01 00:00:00 */
#define	BSE_MAX_TIME			(2147483647)	/* 2038-01-19 04:14:07 */


/* --- BseSource limits --- */
#define	BSE_MAX_N_ICHANNELS		(32)
#define	BSE_MAX_N_OCHANNELS		(32)


/* --- miscellaneous --- */
#define BSE_MAGIC                       (('B' << 24) | ('S' << 16) | \
	    /* 1112753441 0x42534521 */	 ('E' <<  8) | ('!' <<  0))
/* driver rating */
#define	BSE_RATING_PREFERRED		(100)
#define	BSE_RATING_DEFAULT		(50)
#define	BSE_RATING_FALLBACK		(1)
#define	BSE_RATING_NO_AUTO		(-1)

#define BSE_WAVE_CHUNK_MAX_CHANNELS     (8)
#define BSE_WAVE_CHUNK_PADDING          (8)
#define BSE_DCACHE_BLOCK_SIZE           (4096)
#define BSE_DCACHE_CACHE_MEMORY         (64 * 1024 * 1024)

#endif /* __BSE_CONST_VALUES_H__ */
