// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseengine.hh"
#include "bsepcmdevice.hh"


/* --- typedefs & structures --- */
typedef struct
{
  guint         n_values;	/* bse_engine_block_size() * 2 (stereo) */
  gfloat       *buffer;
  gfloat       *bound;
  Bse::Driver  *driver;
  BsePcmWriter *pcm_writer;
  gboolean      pcm_input_checked;
} BsePCMModuleData;
enum
{
  BSE_PCM_MODULE_JSTREAM_LEFT,
  BSE_PCM_MODULE_JSTREAM_RIGHT,
  BSE_PCM_MODULE_N_JSTREAMS
};
enum
{
  BSE_PCM_MODULE_OSTREAM_LEFT,
  BSE_PCM_MODULE_OSTREAM_RIGHT,
  BSE_PCM_MODULE_N_OSTREAMS
};


/* --- functions --- */
static gboolean
bse_pcm_module_poll (gpointer       data,
		     guint          n_values,
		     glong         *timeout_p,
		     guint          n_fds,
		     const GPollFD *fds,
		     gboolean       revents_filled)
{
  BsePCMModuleData *mdata = (BsePCMModuleData*) data;
  return mdata->driver->pcm_check_io (timeout_p);
}

static void
bse_pcm_omodule_process (BseModule *module,
			 guint      n_values)
{
  BsePCMModuleData *mdata = (BsePCMModuleData*) module->user_data;
  gfloat *d = mdata->buffer;
  gfloat *b = mdata->bound;
  const gfloat *src;
  guint i;

  assert_return (n_values == mdata->n_values / BSE_PCM_MODULE_N_JSTREAMS);

  if (BSE_MODULE_JSTREAM (module, BSE_PCM_MODULE_JSTREAM_LEFT).n_connections)
    src = BSE_MODULE_JBUFFER (module, BSE_PCM_MODULE_JSTREAM_LEFT, 0);
  else
    src = bse_engine_const_values (0);
  d = mdata->buffer;
  do { *d = *src++; d += 2; } while (d < b);
  for (i = 1; i < BSE_MODULE_JSTREAM (module, BSE_PCM_MODULE_JSTREAM_LEFT).n_connections; i++)
    {
      src = BSE_MODULE_JBUFFER (module, BSE_PCM_MODULE_JSTREAM_LEFT, i);
      d = mdata->buffer;
      do { *d += *src++; d += 2; } while (d < b);
    }

  if (BSE_MODULE_JSTREAM (module, BSE_PCM_MODULE_JSTREAM_RIGHT).n_connections)
    src = BSE_MODULE_JBUFFER (module, BSE_PCM_MODULE_JSTREAM_RIGHT, 0);
  else
    src = bse_engine_const_values (0);
  d = mdata->buffer + 1;
  do { *d = *src++; d += 2; } while (d < b);
  for (i = 1; i < BSE_MODULE_JSTREAM (module, BSE_PCM_MODULE_JSTREAM_RIGHT).n_connections; i++)
    {
      src = BSE_MODULE_JBUFFER (module, BSE_PCM_MODULE_JSTREAM_RIGHT, i);
      d = mdata->buffer + 1;
      do { *d += *src++; d += 2; } while (d < b);
    }

  mdata->driver->pcm_write (mdata->n_values, mdata->buffer);
  if (mdata->pcm_writer)
    bse_pcm_writer_write (mdata->pcm_writer, mdata->n_values, mdata->buffer,
                          bse_module_tick_stamp (module));
}

static void
bse_pcm_module_data_free (gpointer        data,
			  const BseModuleClass *klass)
{
  BsePCMModuleData *mdata = (BsePCMModuleData*) data;

  g_free (mdata->buffer);
  g_free (mdata);
}

static BseModule*
bse_pcm_omodule_insert (Bse::Driver *pcm_driver, BsePcmWriter *writer, BseTrans *trans)
{
  static const BseModuleClass pcm_omodule_class = {
    0,				/* n_istreams */
    BSE_PCM_MODULE_N_JSTREAMS,	/* n_jstreams */
    0,				/* n_ostreams */
    bse_pcm_omodule_process,	/* process */
    NULL,                       /* process_defer */
    NULL,                       /* reset */
    bse_pcm_module_data_free,	/* free */
    Bse::ModuleFlag::CHEAP,	/* cost */
  };
  BsePCMModuleData *mdata;
  BseModule *module;

  assert_return (pcm_driver && pcm_driver->pcm_frequency(), NULL);
  assert_return (pcm_driver->writable(), NULL);
  assert_return (trans != NULL, NULL);

  mdata = g_new0 (BsePCMModuleData, 1);
  mdata->n_values = bse_engine_block_size () * BSE_PCM_MODULE_N_JSTREAMS;
  mdata->buffer = g_new0 (gfloat, mdata->n_values);
  mdata->bound = mdata->buffer + mdata->n_values;
  mdata->driver = pcm_driver;
  mdata->pcm_writer = writer;
  module = bse_module_new (&pcm_omodule_class, mdata);

  bse_trans_add (trans,
		 bse_job_integrate (module));
  bse_trans_add (trans,
		 bse_job_set_consumer (module, TRUE));
  bse_trans_add (trans,
		 bse_job_add_poll (bse_pcm_module_poll, mdata, NULL, 0, NULL));

  return module;
}

static void
bse_pcm_omodule_remove (BseModule *pcm_module,
			BseTrans  *trans)
{
  assert_return (pcm_module != NULL);
  assert_return (trans != NULL);

  BsePCMModuleData *mdata = (BsePCMModuleData*) pcm_module->user_data;
  bse_trans_add (trans,
		 bse_job_remove_poll (bse_pcm_module_poll, mdata));
  bse_trans_add (trans,
		 bse_job_discard (pcm_module));
}

static gboolean
pcm_imodule_check_input (gpointer data)         /* UserThread */
{
  Bse::ServerImpl::instance().require_pcm_input();
  return false;
}

static void
bse_pcm_imodule_reset (BseModule *module)       /* EngineThread */
{
  BsePCMModuleData *mdata = (BsePCMModuleData*) module->user_data;
  if (!mdata->pcm_input_checked)
    {
      mdata->pcm_input_checked = TRUE;
      /* queue a job into the BSE core for immediate execution */
      bse_idle_now (pcm_imodule_check_input, NULL);
    }
}

static void
bse_pcm_imodule_process (BseModule *module,     /* EngineThread */
			 guint      n_values)
{
  BsePCMModuleData *mdata = (BsePCMModuleData*) module->user_data;
  gfloat *left = BSE_MODULE_OBUFFER (module, BSE_PCM_MODULE_OSTREAM_LEFT);
  gfloat *right = BSE_MODULE_OBUFFER (module, BSE_PCM_MODULE_OSTREAM_RIGHT);
  gsize l;

  assert_return (n_values <= mdata->n_values / BSE_PCM_MODULE_N_OSTREAMS);

  if (mdata->driver->readable())
    {
      l = mdata->driver->pcm_read (mdata->n_values, mdata->buffer);
      assert_return (l == mdata->n_values);
    }
  else
    memset (mdata->buffer, 0, mdata->n_values * sizeof (gfloat));

  /* due to suspend/resume, we may be called with partial read requests */
  const gfloat *s = mdata->buffer + mdata->n_values - (n_values * BSE_PCM_MODULE_N_OSTREAMS);
  const gfloat *b = mdata->bound;
  do
    {
      *left++ = *s++;
      *right++ = *s++;
    }
  while (s < b);
}

static BseModule*
bse_pcm_imodule_insert (Bse::Driver *pcm_driver, BseTrans *trans)
{
  static const BseModuleClass pcm_imodule_class = {
    0,				/* n_istreams */
    0,				/* n_jstreams */
    BSE_PCM_MODULE_N_OSTREAMS,	/* n_ostreams */
    bse_pcm_imodule_process,	/* process */
    NULL,                       /* process_defer */
    bse_pcm_imodule_reset,      /* reset */
    bse_pcm_module_data_free,	/* free */
    Bse::ModuleFlag::EXPENSIVE,		/* cost */
  };
  BsePCMModuleData *mdata;
  BseModule *module;

  assert_return (pcm_driver && pcm_driver->pcm_frequency(), NULL);
  assert_return (trans != NULL, NULL);

  mdata = g_new0 (BsePCMModuleData, 1);
  mdata->n_values = bse_engine_block_size () * BSE_PCM_MODULE_N_OSTREAMS;
  mdata->buffer = g_new0 (gfloat, mdata->n_values);
  mdata->bound = mdata->buffer + mdata->n_values;
  mdata->driver = pcm_driver;
  mdata->pcm_writer = NULL;
  module = bse_module_new (&pcm_imodule_class, mdata);

  bse_trans_add (trans,
		 bse_job_integrate (module));

  return module;
}

static void
bse_pcm_imodule_remove (BseModule *pcm_module,
			BseTrans  *trans)
{
  assert_return (pcm_module != NULL);
  assert_return (trans != NULL);

  bse_trans_add (trans,
		 bse_job_discard (pcm_module));
}
