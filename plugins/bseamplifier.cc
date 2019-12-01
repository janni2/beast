// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseamplifier.genidl.hh"
#include <bse/bsemathsignal.hh>

namespace Bse {

class Amplifier : public AmplifierBase {
  /* amplifier module implementation */
  class Module : public SynthesisModule {
    /* configuration: */
    double al1, al2, cl1, cl2;
    double ocs, bl;
    bool ctrl_mul, ctrl_exp;
  public:
    void
    config (AmplifierProperties *params)
    {
      al1 = params->alevel1 * 0.01;
      al2 = params->alevel2 * 0.01;
      cl1 = params->clevel1 * 0.01;
      cl2 = params->clevel2 * 0.01;
      ctrl_mul = params->ctrl_mul;
      ctrl_exp = params->ctrl_exp;
      if (ctrl_mul)
        ocs = params->ostrength * 0.01;
      else
        ocs = params->ostrength * 0.5 * 0.01;
      bl = params->base_level * 0.01;
      double master = params->master_volume;
      al1 *= master;
      al2 *= master;
    }
    void
    reset ()
    {}
    /* process() function special cases */
#define ACASE_MASK                      (3 << 0)
    static const int ACASE_A1n_A2n     = 0 << 0;        /* !audio1 && !audio2 */
    static const int ACASE_A1n_A2y     = 1 << 0;        /* !audio1 &&  audio2 */
    static const int ACASE_A1y_A2n     = 2 << 0;         /*  audio1 && !audio2 */
    static const int ACASE_A1b_A2b     = 3 << 0;         /*  audio1 &&  audio2 && balance */
#define CCASE_MASK                      (3 << 2)
    static const int CCASE_C1m_C2m     = 0 << 2;        /*  control1 &&  control2 && multiplication */
    static const int CCASE_C1n_C2y     = 1 << 2;        /* !control1 &&  control2 */
    static const int CCASE_C1y_C2n     = 2 << 2;        /*  control1 && !control2 */
    static const int CCASE_C1b_C2b     = 3 << 2;        /*  control1 &&  control2 && balance */
    static const int WITH_EXP_CONTROLS = 16;
    /* multi-case processing loop */
    template<int FLAGS, bool SIMPLE_CONTROL> inline void
    process_loop (unsigned int n_values)
    {
      const float *au1in = istream (ICHANNEL_AUDIO_IN1).values;
      const float *au2in = istream (ICHANNEL_AUDIO_IN2).values;
      const float *cv1in = istream (ICHANNEL_CTRL_IN1).values;
      const float *cv2in = istream (ICHANNEL_CTRL_IN2).values;
      float *audio_out = ostream (OCHANNEL_AUDIO_OUT).values;
      float *const audio_bound = audio_out + n_values;
      const int ACASE = FLAGS & ACASE_MASK, CCASE = FLAGS & CCASE_MASK;
      do
        {
          double cv_sum;
          if (SIMPLE_CONTROL)                           /* no control inputs */
            cv_sum = bl;
          else
            {
              if (CCASE == CCASE_C1b_C2b)	        /* control input, cv1 and/or cv2 */
                cv_sum = cl1 * *cv1in++ + cl2 * *cv2in++;
              else if (CCASE == CCASE_C1m_C2m)
                {
                  double c1 = cl1 * *cv1in++, c2 = cl2 * *cv2in++;
                  cv_sum = c1 > 0 && c2 > 0 ? c1 * c2 : 0;
                }
              else if (CCASE == CCASE_C1y_C2n)
                cv_sum = cl1 * *cv1in++;
              else if (CCASE == CCASE_C1n_C2y)
                cv_sum = cl2 * *cv2in++;

              if (G_UNLIKELY (cv_sum < 0))
                cv_sum = 0;
              else
                cv_sum *= ocs;
              if (FLAGS & WITH_EXP_CONTROLS)            /* exponential controls */
                cv_sum = bse_approx_qcircle4 (cv_sum);
              cv_sum += bl;
              if (G_UNLIKELY (cv_sum > 1.0))
                cv_sum = 1.0;
            }

          double au_out; // never unused
          if (ACASE == ACASE_A1b_A2b)	        /* audio input, au1 and/or au2 */
            au_out = al1 * *au1in++ + al2 * *au2in++;
          else if (ACASE == ACASE_A1y_A2n)
            au_out = al1 * *au1in++;
          else if (ACASE == ACASE_A1n_A2y)
            au_out = al2 * *au2in++;

          au_out *= cv_sum;
          *audio_out++ = au_out;
        }
      while (audio_out < audio_bound);
    }
    void
    process (unsigned int n_values)
    {
      guint mode = 0;
      if (istream (ICHANNEL_AUDIO_IN1).connected &&
          istream (ICHANNEL_AUDIO_IN2).connected)
        mode |= ACASE_A1b_A2b;
      else if (istream (ICHANNEL_AUDIO_IN1).connected) /* !audio2 */
        mode |= ACASE_A1y_A2n;
      else if (istream (ICHANNEL_AUDIO_IN2).connected) /* !audio1 */
        mode |= ACASE_A1n_A2y;
      else /* !audio1 && !audio2 */
        {
          ostream_set (OCHANNEL_AUDIO_OUT, const_values (0));
          return;
        }
      bool simple_control = false;
      if (istream (ICHANNEL_CTRL_IN1).connected &&
          istream (ICHANNEL_CTRL_IN2).connected)
        mode |= ctrl_mul ? CCASE_C1m_C2m : CCASE_C1b_C2b;
      else if (istream (ICHANNEL_CTRL_IN1).connected) /* !control2 */
        mode |= CCASE_C1y_C2n;
      else if (istream (ICHANNEL_CTRL_IN2).connected) /* !control1 */
        mode |= CCASE_C1n_C2y;
      else /* !control1 && !control2 */
        simple_control = true;
      if (!simple_control && ctrl_exp)
        mode |= WITH_EXP_CONTROLS;

      if (simple_control)
        switch (mode)
          {
          case ACASE_A1n_A2y: process_loop <ACASE_A1n_A2y, true> (n_values); break;
          case ACASE_A1y_A2n: process_loop <ACASE_A1y_A2n, true> (n_values); break;
          case ACASE_A1b_A2b: process_loop <ACASE_A1b_A2b, true> (n_values); break;
          }
      else /* !simple_control */
        processlooptable_[mode] (this, n_values);
    }
    using ProcessFunc = void (*) (Module*, unsigned int);
    static std::array<ProcessFunc, 32> processlooptable_;
  };
  template<int FLAGS, bool SIMPLE_CONTROL> static inline void
  static_process_loop (Module *amp, unsigned int n_values)
  {
    amp->process_loop<FLAGS, SIMPLE_CONTROL> (n_values);
  }
  static std::array<Amplifier::Module::ProcessFunc, 32>
  mk_process_loop_table ()
  {
    return Bse::make_jump_table<32> ([] (auto CINDEX) {
        if constexpr (Amplifier::Module::ACASE_A1n_A2n != (CINDEX & ACASE_MASK))
          {
            return static_process_loop<CINDEX, false>;
          }
        // unused cases, i.e. simple_control
        return Amplifier::Module::ProcessFunc (nullptr);
      });
  }
protected:
  bool
  property_changed (AmplifierPropertyID prop_id)
  {
    switch (prop_id)
      {
        /* implement special handling of GUI properties */
      case PROP_ALEVEL1:
      case PROP_ALEVEL2:
        abalance = bse_balance_get (alevel1, alevel2);
        notify ("abalance");
        break;
      case PROP_ABALANCE:
        bse_balance_set (abalance, &alevel1, &alevel2);
        notify ("alevel1");
        notify ("alevel2");
        break;
      case PROP_CLEVEL1:
      case PROP_CLEVEL2:
        cbalance = bse_balance_get (clevel1, clevel2);
        notify ("cbalance");
        break;
      case PROP_CBALANCE:
        bse_balance_set (cbalance, &clevel1, &clevel2);
        notify ("clevel1");
        notify ("clevel2");
        break;
      case PROP_AUDIO_BALANCE:          // compat 0.5.4
        alevel1 = alevel2 = 100;
        bse_balance_set (audio_balance, &alevel1, &alevel2);
        abalance = bse_balance_get (alevel1, alevel2);
        notify ("alevel1");
        notify ("alevel2");
        notify ("abalance");
        break;
      case PROP_CTRL_BALANCE:           // compat 0.5.4
        clevel1 = clevel2 = 100;
        bse_balance_set (ctrl_balance, &clevel1, &clevel2);
        cbalance = bse_balance_get (clevel1, clevel2);
        notify ("clevel1");
        notify ("clevel2");
        notify ("cbalance");
        break;
      case PROP_CTRL_STRENGTH_F:        // compat 0.5.4
        ostrength = ctrl_strength_f * 100.0;
        notify ("ostrength");
        break;
      case PROP_MASTER_GAIN_F:          // compat 0.5.4
        olevel = master_gain_f * 100.0;
        notify ("olevel");
        /* fall through */
      case PROP_OLEVEL:                 // compat 0.5.4
        master_volume = olevel * 0.01;
        notify ("master_volume");
        break;
      case PROP_AUDIO_GAIN_F:           // compat 0.5.4
        base_level = audio_gain_f * 100.0;
        notify ("base_level");
        break;
      default: ;
      }
    return false;
  }
  void
  compat_setup (guint          vmajor,
                guint          vminor,
                guint          vmicro)
  {
    if (BSE_VERSION_CMP (vmajor, vminor, vmicro, 0, 6, 2) <= 0)
      set ("olevel", 100.0, NULL);
    if (BSE_VERSION_CMP (vmajor, vminor, vmicro, 0, 5, 4) <= 0)
      set ("ctrl_exp", TRUE,
           "audio_gain_f", 0.5,
           "ctrl_mul", FALSE,
           NULL);
  }
  void
  restore_finished (guint          vmajor,
                    guint          vminor,
                    guint          vmicro)
  {
    if (BSE_VERSION_CMP (vmajor, vminor, vmicro, 0, 7, 1) <= 0)
      if (ctrl_mul)
        set ("ostrength", ostrength * 0.5, NULL);
  }
public:
  /* implement creation and config methods for synthesis Module */
  BSE_EFFECT_INTEGRATE_MODULE (Amplifier, Module, AmplifierProperties);
};

std::array<Amplifier::Module::ProcessFunc, 32> Amplifier::Module::processlooptable_ = Amplifier::mk_process_loop_table();

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (Amplifier);

} // Bse

