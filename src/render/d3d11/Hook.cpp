#include "D3d11.h"

#include "common.h"

#include "global/Global.h"

namespace AlphaRing::Render::D3d11 {
    DefDetourFunction(HRESULT, __stdcall, DrawIndexed, ID3D11DeviceContext* p_context,
                      const UINT IndexCount, const UINT StartIndexLocation, const INT  BaseVertexLocation) {
        if (AlphaRing::Global::Global()->wireframe) {
            Graphics()->SetWireframe();
#ifdef NEW_WIREFRAME
            D3D11_PRIMITIVE_TOPOLOGY topology;
            p_context->IAGetPrimitiveTopology(&topology);

            p_context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
            ppOriginal_DrawIndexed(p_context, IndexCount, StartIndexLocation, BaseVertexLocation);

            p_context->IASetPrimitiveTopology(topology);
#endif
        }

        return ppOriginal_DrawIndexed(p_context, IndexCount, StartIndexLocation, BaseVertexLocation);
    }

    bool CreateHook() {
        // The DrawIndexed detour taxes every draw call just to support the
        // wireframe debug toggle. Only install it when explicitly requested.
        char buf[8] = {};
        if (GetEnvironmentVariableA("ALPHARING_WIREFRAME", buf, sizeof(buf)) == 0 || buf[0] == '0')
            return true;

        return Hook::Detour({
            {GetFunction(73), DrawIndexed, (void **) &ppOriginal_DrawIndexed},
        });
    }
}