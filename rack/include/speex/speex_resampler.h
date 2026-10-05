// SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-Oroboro-Rack-Bridge-Exception (rack/LICENSE-EXCEPTION.md)
/* The names of speex's resampler that Rack plugins use with Rack's
 * `dsp::SampleRateConverter` (Rack's resampler is speex's, and its header
 * brings these in): the quality levels it takes, and the errors. The SDK's
 * own declarations: its converter is its own (rack_dsp.hpp), and speex's
 * functions aren't here. */
#ifndef SPEEX_RESAMPLER_H
#define SPEEX_RESAMPLER_H

#define SPEEX_RESAMPLER_QUALITY_MAX 10
#define SPEEX_RESAMPLER_QUALITY_MIN 0
#define SPEEX_RESAMPLER_QUALITY_DEFAULT 4
#define SPEEX_RESAMPLER_QUALITY_VOIP 3
#define SPEEX_RESAMPLER_QUALITY_DESKTOP 5

enum {
	RESAMPLER_ERR_SUCCESS = 0,
	RESAMPLER_ERR_ALLOC_FAILED = 1,
	RESAMPLER_ERR_BAD_STATE = 2,
	RESAMPLER_ERR_INVALID_ARG = 3,
	RESAMPLER_ERR_PTR_OVERLAP = 4,
	RESAMPLER_ERR_OVERFLOW = 5,
	RESAMPLER_ERR_MAX_ERROR
};

#endif
