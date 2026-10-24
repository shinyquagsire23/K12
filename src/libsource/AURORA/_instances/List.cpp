// Explicit-instantiation home for the Aurora container, with a declaration for every element type
// it is instantiated over.
#if KOTOR1
#define K1_AURORA_LIST
#endif
#include "List.h"
#include "Vector.h"
#include "Quaternion.h"
#include "Plane.h"
#include "aurgrass.h"

namespace Aurora { enum AuroraBlendModes { }; }

// Pointer elements need only a forward declaration.
struct AnimRun;
class Animation;
struct BSPNode;
class Binding;
class CAurFont;
class CAurObject;
struct CAurRoom;
class CAurTextureBasic;
class CAurTriangleBin;
class CallBack;
class Gob;
class LightningEmitter;
class Material;
class MdlNode;
class Model;
class NvFaceInfo;
class OpenResource;
class Part;
class PartBeam;
class PartEmitter;
class PartLight;
class PartShadow;
class PartTriMesh;
class PartTrigger;
class Particle;
class PostedString;
class Scene;
class TileData;
class TimeTrigger;
class TraceString;
class VertexProgram;

class CAurTexture;
template <class T> class SafePointer;

#include "auranim.h"

#if KOTOR1
template <> struct K1ListFirstGrow<Vector> { enum { value = 16 }; };
template <> struct K1ListFirstGrow<int> { enum { value = 16 }; };
#endif
#include "Matrix.h"
#include "skinvertexweight.h"

template <> void List<CAurGrassInfo>::AddUnique(CAurGrassInfo) = delete;
template <> void List<Matrix>::AddUnique(Matrix) = delete;
template <> void List<NewController>::AddUnique(NewController) = delete;
template <> void List<Plane>::AddUnique(Plane) = delete;
template <> void List<Quaternion>::AddUnique(Quaternion) = delete;
template <> void List<SkinVertexWeight>::AddUnique(SkinVertexWeight) = delete;
template <> void List<Vector>::AddUnique(Vector) = delete;
template <> void List<List<int> >::AddUnique(List<int>) = delete;

template class List<AnimRun*>;
template class List<Animation*>;
template class List<Aurora::AuroraBlendModes>;
template class List<BSPNode*>;
template class List<Binding*>;
template class List<CAurFont*>;
template class List<CAurGrassInfo>;
template class List<CAurObject*>;
template class List<CAurRoom*>;
template <> void List<CAurTextureBasic*>::AddUnique(CAurTextureBasic* item)
{
    if (!Contains(item))
        Add(item);
}
template class List<CAurTextureBasic*>;
template class List<CAurTriangleBin*>;
template class List<CallBack*>;
template class List<Gob*>;
template class List<LightningEmitter*>;
template class List<Material*>;
template class List<Matrix>;
template class List<MdlNode*>;
template class List<Model*>;
template class List<NewController>;
template class List<NvFaceInfo*>;
template class List<OpenResource*>;
template class List<Part*>;
template class List<PartBeam*>;
template class List<PartEmitter*>;
template class List<PartLight*>;
template class List<SafePointer<CAurObject>*>;
template class List<SafePointer<CAurTexture>*>;
template class List<PartShadow*>;
template class List<PartTriMesh*>;
template class List<PartTrigger*>;
template class List<Particle*>;
template class List<PostedString*>;
template class List<Quaternion>;
template class List<Plane>;
template class List<Scene*>;
template class List<SkinVertexWeight>;
template class List<TileData*>;
template class List<TimeTrigger*>;
template class List<TraceString*>;
template class List<Vector>;
template class List<VertexProgram*>;
template class List<bool>;
template class List<char*>;
template class List<float>;
template class List<int>;
template class List<List<int> >;
template class List<unsigned int>;
template class List<unsigned int*>;
template class List<void*>;
