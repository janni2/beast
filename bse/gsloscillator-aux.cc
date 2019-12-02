// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html

template<size_t OSC_FLAGS> static void
oscillator_process_variants (GslOscData  *osc,
                             uint         n_values,
                             const float *ifreq,
                             const float *mod_in,
                             const float *sync_in,
                             const float *pwm_in,
                             float       *mono_out,
                             float       *sync_out)
{
  constexpr const bool ISYNC1_OSYNC0 = (OSC_FLAGS & OSC_FLAG_ISYNC) && !(OSC_FLAGS & OSC_FLAG_OSYNC);
  constexpr const bool ISYNC1_OSYNC1 = (OSC_FLAGS & OSC_FLAG_ISYNC) && (OSC_FLAGS & OSC_FLAG_OSYNC);
  constexpr const bool ISYNC0_OSYNC1 = (OSC_FLAGS & OSC_FLAG_OSYNC) && !(OSC_FLAGS & OSC_FLAG_ISYNC);
  constexpr const bool WITH_OSYNC = OSC_FLAGS & OSC_FLAG_OSYNC;
  constexpr const bool WITH_FREQ = OSC_FLAGS & OSC_FLAG_FREQ;
  constexpr const bool WITH_SMOD = OSC_FLAGS & OSC_FLAG_SELF_MOD;
  constexpr const bool WITH_LMOD = OSC_FLAGS & OSC_FLAG_LINEAR_MOD;
  constexpr const bool WITH_EMOD = OSC_FLAGS & OSC_FLAG_EXP_MOD;
  constexpr const bool WITH_PWM_MOD = OSC_FLAGS & OSC_FLAG_PWM_MOD;
  constexpr const bool PULSE_OSC = OSC_FLAGS & OSC_FLAG_PULSE_OSC;
  float last_sync_level = osc->last_sync_level;
  float last_pwm_level = osc->last_pwm_level;
  double last_freq_level = osc->last_freq_level;
  const double transpose = osc->config.transpose_factor;
  const double fine_tune = bse_cent_tune_fast (osc->config.fine_tune);
  uint32 cur_pos = osc->cur_pos;
  uint32 last_pos = osc->last_pos;
  uint32 sync_pos, pos_inc;
  float posm_strength, self_posm_strength;
  float *boundary = mono_out + n_values;
  GslOscWave *wave = &osc->wave;

  pos_inc = bse_dtoi (osc->last_freq_level * transpose * fine_tune * wave->freq_to_step);
  sync_pos = osc->config.phase * wave->phase_to_pos;
  posm_strength = pos_inc * osc->config.fm_strength;
  self_posm_strength = pos_inc * osc->config.self_fm_strength;

  /* do the mixing */
  do
    {
      gfloat v;

      /* handle syncs
       */
      if (ISYNC1_OSYNC0)	/* input sync only */
        {
          gfloat sync_level = *sync_in++;
          if (UNLIKELY (BSE_SIGNAL_RAISING_EDGE (last_sync_level, sync_level)))
            cur_pos = sync_pos;
          last_sync_level = sync_level;
        }
      else if (ISYNC1_OSYNC1)	/* input and output sync */
        {
          gfloat sync_level = *sync_in++;
          if (UNLIKELY (BSE_SIGNAL_RAISING_EDGE (last_sync_level, sync_level)))
            {
              cur_pos = sync_pos;
              *sync_out++ = 1.0;
            }
          else /* figure output sync position */
            {
              guint is_sync = (sync_pos <= cur_pos) + (last_pos < sync_pos) + (cur_pos < last_pos);
              *sync_out++ = is_sync >= 2 ? 1.0 : 0.0;
            }
          last_sync_level = sync_level;
        }
      else if (ISYNC0_OSYNC1)	/* output sync only */
        {
          /* figure output sync position */
          guint is_sync = (sync_pos <= cur_pos) + (last_pos < sync_pos) + (cur_pos < last_pos);
          *sync_out++ = is_sync >= 2 ? 1.0 : 0.0;
        }

      /* track frequency changes
       */
      if (WITH_FREQ)
        {
          gdouble freq_level = *ifreq++;
          freq_level = BSE_SIGNAL_TO_FREQ (freq_level);
          if (UNLIKELY (BSE_SIGNAL_FREQ_CHANGED (last_freq_level, freq_level)))
            {
              gdouble transposed_freq = transpose * freq_level;
              if (UNLIKELY (transposed_freq <= wave->min_freq || transposed_freq > wave->max_freq))
                {
                  gdouble fcpos, flpos;
                  const gfloat *orig_values = wave->values;

                  fcpos = cur_pos * wave->ifrac_to_float;
                  flpos = last_pos * wave->ifrac_to_float;
                  gsl_osc_table_lookup (osc->config.table, transposed_freq, wave);
                  if (orig_values != wave->values)	/* catch non-changes */
                    {
                      last_pos = flpos / wave->ifrac_to_float;
                      cur_pos = fcpos / wave->ifrac_to_float;
                      sync_pos = osc->config.phase * wave->phase_to_pos;
                      pos_inc = bse_dtoi (transposed_freq * fine_tune * wave->freq_to_step);
                      if (PULSE_OSC)
                        {
                          osc->last_pwm_level = 0;
                          osc_update_pwm_offset (osc, osc->last_pwm_level);
                          last_pwm_level = osc->last_pwm_level;
                        }
                    }
                }
              else
                pos_inc = bse_dtoi (transposed_freq * fine_tune * wave->freq_to_step);
              posm_strength = pos_inc * osc->config.fm_strength;
              self_posm_strength = pos_inc * osc->config.self_fm_strength;
              last_freq_level = freq_level;
            }
        }

      /* track pulse witdh modulation
       */
      if (WITH_PWM_MOD)
        {
          gfloat pwm_level = *pwm_in++;
          if (fabs (last_pwm_level - pwm_level) > 1.0 / 65536.0)
            {
              last_pwm_level = pwm_level;
              osc_update_pwm_offset (osc, pwm_level);
            }
        }

      /* output signal calculation
       */
      if (PULSE_OSC)	/* pulse width modulation oscillator */
        {
          guint32 tpos, ipos;
          tpos = cur_pos >> wave->n_frac_bits;
          ipos = (cur_pos - osc->pwm_offset) >> wave->n_frac_bits;
          v = wave->values[tpos] - wave->values[ipos];
          v = (v + osc->pwm_center) * osc->pwm_max;
        }
      else		/* table read out and linear ipol */
        {
          guint32 tpos, ifrac;
          gfloat ffrac, w;
          tpos = cur_pos >> wave->n_frac_bits;
          ifrac = cur_pos & wave->frac_bitmask;
          ffrac = ifrac * wave->ifrac_to_float;
          v = wave->values[tpos];
          w = wave->values[tpos + 1];
          v *= 1.0 - ffrac;
          w *= ffrac;
          v += w;
        }
      /* v = value_out done */
      *mono_out++ = v;

      /* position increment
       */
      if (WITH_OSYNC)
        last_pos = cur_pos;
      if (WITH_SMOD)		/* self modulation */
        cur_pos += self_posm_strength * v;
      if (WITH_LMOD)		/* linear fm */
        {
          gfloat mod_level = *mod_in++;
          cur_pos += pos_inc + posm_strength * mod_level;
        }
      else if (WITH_EMOD)	/* exponential fm */
        {
          gfloat mod_level = *mod_in++;
          cur_pos += pos_inc * bse_approx5_exp2 (osc->config.fm_strength * mod_level);
        }
      else			/* no modulation */
        cur_pos += pos_inc;
    }
  while (mono_out < boundary);

  osc->last_pos = WITH_OSYNC ? last_pos : cur_pos;
  osc->cur_pos = cur_pos;
  osc->last_sync_level = last_sync_level;
  osc->last_freq_level = last_freq_level;
  osc->last_pwm_level = last_pwm_level;
}

#undef ISYNC1_OSYNC0
#undef ISYNC1_OSYNC1
#undef ISYNC0_OSYNC1
#undef WITH_OSYNC
#undef WITH_FREQ
#undef WITH_SMOD
#undef WITH_LMOD
#undef WITH_EMOD
#undef OSC_FLAGS
