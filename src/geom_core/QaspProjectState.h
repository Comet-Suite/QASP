// NOSA 1.3; see LICENSE. QASP's optional, versioned .vsp3 setup extension.
#ifndef QASP_PROJECT_STATE_H
#define QASP_PROJECT_STATE_H
#include "XmlUtil.h"
// Called while the file's ID remapping context is still active. Upstream readers
// ignore this optional node; resaving in a reader unaware of it can remove it.
void QaspEncodeProjectState( xmlNodePtr &root );
void QaspDecodeProjectState( xmlNodePtr &root );
#endif
