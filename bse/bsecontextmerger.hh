// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_CONTEXT_MERGER_H__
#define __BSE_CONTEXT_MERGER_H__

#include <bse/bsecontainer.hh>

/* --- object type macros --- */
#define BSE_TYPE_CONTEXT_MERGER              (BSE_TYPE_ID (BseContextMerger))
#define BSE_CONTEXT_MERGER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_CONTEXT_MERGER, BseContextMerger))
#define BSE_CONTEXT_MERGER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_CONTEXT_MERGER, BseContextMergerClass))
#define BSE_IS_CONTEXT_MERGER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_CONTEXT_MERGER))
#define BSE_IS_CONTEXT_MERGER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_CONTEXT_MERGER))
#define BSE_CONTEXT_MERGER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_CONTEXT_MERGER, BseContextMergerClass))
#define BSE_CONTEXT_MERGER_N_IOPORTS (8)
/* --- object structures --- */
struct BseContextMerger : BseContainer {
  guint merge_context;
};
struct BseContextMergerClass : BseContainerClass
{};

void	bse_context_merger_set_merge_context	(BseContextMerger	*self,
						 guint			 merge_context);
namespace Bse {

class ContextMergerImpl : public ContainerImpl, public virtual ContextMergerIface {
protected:
  virtual             ~ContextMergerImpl           ();
public:
  explicit             ContextMergerImpl           (BseObject*);
};

} // Bse

#endif /* __BSE_CONTEXT_MERGER_H__ */
