// Explicit instantiations of the templated resource handle, one per resource type.

#include "CResHelper.h"

#include "res2da.h"
#include "res4pc.h"
#include "resdds.h"
#include "resfnt.h"
#include "reslip.h"
#include "resltr.h"
#include "reslyt.h"
#include "resmdl.h"
#include "resmdx.h"
#include "resncs.h"
#include "resnss.h"
#include "resplt.h"
#include "resssf.h"
#include "restga.h"
#include "restpc.h"
#include "restxi.h"
#include "resvis.h"
#include "reswave.h"
#include "resare.h"
#include "resifo.h"

template class CResHelper<CRes2DA, 2017>;

template class CResHelper<CRes4PC, 2059>;

template class CResHelper<CResDDS, 2033>;

template class CResHelper<CResFNT, 2005>;

template class CResHelper<CResLIP, 3004>;

template class CResHelper<CResLTR, 2036>;

template class CResHelper<CResLYT, 3000>;

template class CResHelper<CResMDL, 2002>;

template class CResHelper<CResMDX, 3008>;

template class CResHelper<CResNCS, 2010>;

template class CResHelper<CResNSS, 2009>;

template class CResHelper<CResPLT, 6>;

template class CResHelper<CResSSF, 2060>;

template class CResHelper<CResTGA, 3>;

template class CResHelper<CResTPC, 3007>;

template class CResHelper<CResTXI, 2022>;

template class CResHelper<CResVIS, 3001>;

template class CResHelper<CResWave, 4>;

template class CResHelper<CResARE, 2012>;

template class CResHelper<CResIFO, 2014>;

