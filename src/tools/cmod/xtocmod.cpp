#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <d3dx9.h>

using namespace std;

static IDirect3D9* g_d3d = NULL;
static IDirect3DDevice9* g_d3dDev = NULL;
static HWND g_mainWindow = NULL;

char* D3DErrorString(HRESULT);    
void ShowD3DErrorMessage(char* info, HRESULT hr);


struct VertexAttribute
{
    enum {
        Position = 0,
        Color0   = 1,
        Color1   = 2,
        Normal   = 3,
        Tangent  = 4,
        Texture0 = 5,
        Texture1 = 6,
        Texture2 = 7,
        Texture3 = 8,
        MaxAttribute = 9,
        InvalidAttribute  = -1,
    };

    enum Format
    {
        Float1 = 0,
        Float2 = 1,
        Float3 = 2,
        Float4 = 3,
        UByte4 = 4,
        InvalidFormat = -1,
    };

    unsigned int offset;
    Format format;
};

char* AttribFormatNames[] =
{ "f1", "f2", "f3", "f4", "ub4" };

char* AttribNames[] = { 
    "position",
    "color0",
    "color1",
    "normal",
    "tangent",
    "texcoord0",
    "texcoord1",
    "texcoord2",
    "texcoord3"
};


bool operator==(const D3DCOLORVALUE& c0, const D3DCOLORVALUE& c1)
{
    return (c0.r == c1.r && c0.g == c1.g && c0.b == c1.b && c0.a == c1.a);
}


bool operator<(const D3DCOLORVALUE& c0, const D3DCOLORVALUE& c1)
{
    if (c0.r == c1.r)
    {
        if (c0.g == c1.g)
        {
            if (c0.b == c1.b)
                return c0.a < c1.a;
            else
                return c0.b < c1.b;
        }
        else
        {
            return c0.g < c1.g;
        }
    }
    else
    {
        return c0.r < c1.r;
    }
}


bool operator==(const D3DXMATERIAL& mat0, const D3DXMATERIAL& mat1)
{
    // Compare the texture filenames for equality, safely handling
    // null filenames.
    bool sameTex;
    if (mat0.pTextureFilename == NULL)
    {
        sameTex = (mat1.pTextureFilename == NULL);
    }
    else if (mat1.pTextureFilename == NULL)
    {
        sameTex = false;
    }
    else
    {
        sameTex = (strcmp(mat0.pTextureFilename, mat1.pTextureFilename) == 0);
    }

    return (mat0.MatD3D.Diffuse == mat1.MatD3D.Diffuse &&
            mat0.MatD3D.Ambient == mat1.MatD3D.Ambient &&
            mat0.MatD3D.Specular == mat1.MatD3D.Specular &&
            mat0.MatD3D.Emissive == mat1.MatD3D.Emissive &&
            mat0.MatD3D.Power == mat1.MatD3D.Power &&
            sameTex);
}


ostream& operator<<(ostream& o, const D3DCOLORVALUE& c)
{
    return (o << c.r << ' ' << c.g << ' ' << c.b);
}


static void render()
{
    g_d3dDev->Clear(0, NULL,
                    D3DCLEAR_TARGET,
                    D3DCOLOR_ARGB(255, 0, 0, 192),  // color
                    0.0f,                           // z
                    0);                             // stencil value
}


template<class T> int checkForFan(DWORD nTris, T* indices)
{
    // Even number of triangles required; pairs of triangles can always
    // be just as efficiently represented as strips, so skip them.
    if (nTris % 2 == 1 || nTris <= 2)
        return -1;

    DWORD i;
    T anchor = indices[0];
    bool isFan = true;
    for (i = 1; i < nTris / 2 && isFan; i++)
    {
        if (indices[i * 2] != anchor)
            isFan = false;
    }

    if (isFan)
        return 0;

    isFan = true;
    anchor = indices[1];
    for (i = 1; i < nTris / 2 && isFan; i++)
    {
        if (indices[i * 2 + 1] != anchor)
            isFan = false;
    }

    if (isFan)
        cout << "fan: nTris=" << nTris << ", anchor=" << anchor << '\n';

    return isFan ? 1 : -1;
}


template<class T> int DumpTriStrip(DWORD nTris,
                                   T* indices,
                                   int materialIndex,
                                   ostream& meshfile)
{
    meshfile << "tristrip ";
    meshfile << materialIndex << ' ' << (nTris + 2) << '\n';
            
    DWORD indexCount = nTris + 2;

    for (DWORD j = 0; j < indexCount; j++)
    {
        meshfile << indices[j] << ' ';
        if (j == indexCount - 1 || j % 12 == 11)
            meshfile << '\n';
    }
}



// The D3DX tristrip converter only produces strips, not fans.  It dumps
// fans as strips where every other triangle is degenerate.  We detect such
// strips and output them as fans instead, thus eliminating a bunch of
// degenerate triangles.
template<class T> void DumpTriStripAsFan(DWORD nTris,
                                         T* indices,
                                         int materialIndex,
                                         DWORD anchorOffset,
                                         ostream& meshfile)
{
    meshfile << "trifan ";
    meshfile << materialIndex << ' ' << (nTris / 2 + 3) << '\n';
            
    DWORD indexCount = nTris + 2;

    T anchor = indices[anchorOffset];
    meshfile << anchor << ' ';

    if (anchorOffset == 1)
    {
        for (int j = (int) indexCount - 1; j >= 0; j--)
        {
            if (indices[j] != anchor)
                meshfile << indices[j] << ' ';
            if (j == 0 || j % 12 == 11)
                meshfile << '\n';
        }
    }
    else if (anchorOffset == 0)
    {
        // D3DX never seems to produce strips where the first vertex is
        // the anchor, but we'll handle it just in case.
        for (int j = 1; j < (int) indexCount; j++)
        {
            if (indices[j] != anchor)
                meshfile << indices[j] << ' ';
            if (j == indexCount - 1 || j % 12 == 11)
                meshfile << '\n';
        }
    }
}


bool StripifyMeshSubset(ID3DXMesh* mesh,
                        DWORD attribId,
                        ostream& meshfile)
{
    // TODO: Fall back to tri lists if the strip size is too small
    // TODO: Detect when a tri fan should be used instead of a tri list

    // Convert to tri strips
    IDirect3DIndexBuffer9* indices = NULL;
    DWORD numIndices = 0;
    ID3DXBuffer* strips = NULL;
    DWORD numStrips = 0;
    HRESULT hr;
    hr = D3DXConvertMeshSubsetToStrips(mesh,
                                       attribId,
                                       0,
                                       &indices,
                                       &numIndices,
                                       &strips,
                                       &numStrips);
    if (FAILED(hr))
    {
        cout << "Stripify failed\n";
        return false;
    }

    cout << "Converted to " << numStrips << " strips\n";
    cout << "Strip buffer size: " << strips->GetBufferSize() << '\n';
    if (numStrips != strips->GetBufferSize() / 4) 
    {
        cout << "Strip count is incorrect!\n";
        return false;
    }

    bool index32 = false;
    {
        D3DINDEXBUFFER_DESC desc;
        indices->GetDesc(&desc);
        if (desc.Format == D3DFMT_INDEX32)
        {
            index32 = true;
        }
        else if (desc.Format == D3DFMT_INDEX16)
        {
            index32 = false;
        }
        else
        {
            cout << "Bad index format.  Strange.\n";
            return false;
        }
    }

    void* indexData = NULL;
    hr = indices->Lock(0, 0, &indexData, D3DLOCK_READONLY);
    if (FAILED(hr))
    {
        cout << "Failed to lock index buffer: " << D3DErrorString(hr) << '\n';
        return false;
    }

    {
        DWORD* stripLengths = reinterpret_cast<DWORD*>(strips->GetBufferPointer());
        int k = 0;
        for (int i = 0; i < numStrips; i++)
        {
            if (stripLengths[i] == 0)
            {
                cout << "Bad triangle strip (length == 0) in mesh!\n";
                return false;
            }

            if (index32)
            {
                DWORD* indices = reinterpret_cast<DWORD*>(indexData) + k;
                int fanStart = checkForFan(stripLengths[i], indices);
                if (fanStart != 1)
                {
                    DumpTriStrip(stripLengths[i], indices, (int) attribId,
                                 meshfile);
                }
                else
                {
                    DumpTriStripAsFan(stripLengths[i], indices, (int) attribId,
                                      fanStart, meshfile);
                }
            }
            else
            {
                WORD* indices = reinterpret_cast<WORD*>(indexData) + k;
                int fanStart = checkForFan(stripLengths[i], indices);
                if (fanStart != 1)
                {
                    DumpTriStrip(stripLengths[i], indices, (int) attribId,
                                 meshfile);
                }
                else
                {
                    DumpTriStripAsFan(stripLengths[i], indices, (int) attribId,
                                      fanStart, meshfile);
                }
            }

            k += stripLengths[i] + 2;
        }

        cout << "k=" << k << ", numIndices=" << numIndices;
        if (index32)
            cout << ", 32-bit indices\n";
        else
            cout << ", 16-bit indices\n";
    }

    return true;
}


void DumpVertexDescription(VertexAttribute vertexMap[],
                           ostream& meshfile)
{
    meshfile << "vertexdesc\n";
    for (int i = 0; i < VertexAttribute::MaxAttribute; i++)
    {
        if (vertexMap[i].format != VertexAttribute::InvalidFormat)
        {
            meshfile << AttribNames[i] << " " <<
                AttribFormatNames[vertexMap[i].format] << " " << '\n';
        }
    }
    meshfile << "end_vertexdesc\n\n";
}


bool DumpMeshVertices(ID3DXMesh* mesh,
                      VertexAttribute vertexMap[],
                      DWORD stride,
                      ostream& meshfile)
{
    IDirect3DVertexBuffer9* vb = NULL;
    HRESULT hr = mesh->GetVertexBuffer(&vb);
    if (FAILED(hr))
    {
        ShowD3DErrorMessage("Getting vertex buffer", hr);
        return false;
    }

    char* vertexData = NULL;
    hr = vb->Lock(0, 0, reinterpret_cast<void**>(&vertexData), D3DLOCK_READONLY);
    if (FAILED(hr) || vertexData == NULL)
    {
        ShowD3DErrorMessage("Locking vertex buffer", hr);
        return false;
    }

    DWORD numVertices = mesh->GetNumVertices();

    meshfile << "vertices " << numVertices << '\n';

    for (DWORD i = 0; i < numVertices; i++)
    {
        for (int attr = 0; attr < VertexAttribute::MaxAttribute; attr++)
        {
            if (vertexMap[attr].format != VertexAttribute::InvalidFormat)
            {
                char* chardata = vertexData + i * stride + vertexMap[attr].offset;
                float* floatdata = reinterpret_cast<float*>(chardata);
                                                            
                switch (vertexMap[attr].format)
                {
                case VertexAttribute::Float1:
                    meshfile << floatdata[0] << ' ';
                    break;
                case VertexAttribute::Float2:
                    meshfile << floatdata[0] << ' ' << floatdata[1] << ' ';
                    break;
                case VertexAttribute::Float3:
                    meshfile << floatdata[0] << ' ' << floatdata[1] << ' ';
                    meshfile << floatdata[2] << ' ';
                    break;
                case VertexAttribute::Float4:
                    meshfile << floatdata[0] << ' ' << floatdata[1] << ' ';
                    meshfile << floatdata[2] << ' ' << floatdata[3];
                    break;
                case VertexAttribute::UByte4:
                    meshfile << (unsigned int) chardata[0] << ' ' <<
                                (unsigned int) chardata[1] << ' ' <<
                                (unsigned int) chardata[2] << ' ' <<
                                (unsigned int) chardata[3] << ' ';
                    break;
                default:
                    break;
                }
            }
        }

        meshfile << '\n';
    }

    vb->Unlock();

    meshfile << '\n';

    return true;
}


bool CreateVertexAttributeMap(D3DVERTEXELEMENT9 declElements[],
                              VertexAttribute vertexMap[])
{
    int i = 0;
    for (i = 0; i < VertexAttribute::MaxAttribute; i++)
    {
        vertexMap[i].offset = 0;
        vertexMap[i].format = VertexAttribute::InvalidFormat;
    }

    unsigned int outputOffset = 0;
    for (i = 0; i < MAX_FVF_DECL_SIZE && declElements[i].Stream != 255; i++)
    {
        int attr = VertexAttribute::InvalidAttribute;
        VertexAttribute::Format format = VertexAttribute::InvalidFormat;
        unsigned int formatSize = 0;

        if (declElements[i].Stream == 0)
        {
            switch (declElements[i].Type)
            {
            case D3DDECLTYPE_FLOAT1:
                format = VertexAttribute::Float1;
                formatSize = 4;
                break;
            case D3DDECLTYPE_FLOAT2:
                format = VertexAttribute::Float2;
                formatSize = 8;
                break;
            case D3DDECLTYPE_FLOAT3:
                format = VertexAttribute::Float3;
                formatSize = 12;
                break;
            case D3DDECLTYPE_FLOAT4:
                format = VertexAttribute::Float4;
                formatSize = 16;
                break;
            case D3DDECLTYPE_UBYTE4:
            case D3DDECLTYPE_UBYTE4N:
                format = VertexAttribute::UByte4;
                formatSize = 4;
                break;
            }

            switch (declElements[i].Usage)
            {
            case D3DDECLUSAGE_POSITION:
                if (declElements[i].UsageIndex == 0)
                {
                    attr = VertexAttribute::Position;
                }
                break;
            case D3DDECLUSAGE_NORMAL:
                if (declElements[i].UsageIndex == 0)
                {
                    attr = VertexAttribute::Normal;
                }
                break;
            case D3DDECLUSAGE_TEXCOORD:
                if (declElements[i].UsageIndex < 4)
                {
                    if (declElements[i].UsageIndex == 0)
                        attr = VertexAttribute::Texture0;
                    else if (declElements[i].UsageIndex == 1)
                        attr = VertexAttribute::Texture1;
                    else if (declElements[i].UsageIndex == 2)
                        attr = VertexAttribute::Texture2;
                    else
                        attr = VertexAttribute::Texture3;
                }
                break;
            case D3DDECLUSAGE_COLOR:
                if (declElements[i].UsageIndex < 2)
                {
                    if (declElements[i].UsageIndex == 0)
                        attr = VertexAttribute::Color0;
                    else
                        attr = VertexAttribute::Color1;
                }
                break;
            case D3DDECLUSAGE_TANGENT:
                if (declElements[i].UsageIndex == 0)
                {
                    attr = VertexAttribute::Tangent;
                }
                break;
            }

            if (attr != VertexAttribute::InvalidAttribute)
            {
                vertexMap[attr].offset = declElements[i].Offset;
                vertexMap[attr].format = format;
            }
        }
    }

    return true;
}


static LRESULT CALLBACK MainWindowProc(HWND hWnd,
                                       UINT uMsg,
                                       WPARAM wParam,
                                       LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_PAINT:
        if (g_d3dDev != NULL)
        {
            //render();
            //g_d3dDev->Present(NULL, NULL, NULL, NULL);
        }
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC) MainWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "xtocmod";
    if (RegisterClass(&wc) == 0)
    {
	MessageBox(NULL,
                   "Failed to register the window class.", "Fatal Error",
                   MB_OK | MB_ICONERROR);
	return NULL;
    }

    DWORD windowStyle = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                         WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    g_mainWindow = CreateWindow("xtocmod",
                                "xtocmod",
                                windowStyle,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                300, 300,
                                NULL,
                                NULL,
                                hInstance,
                                NULL);
    if (g_mainWindow == NULL)
    {
        MessageBox(NULL,
                   "Error creating application window.", "Fatal Error",
                   MB_OK | MB_ICONERROR);
    }

    //ShowWindow(g_mainWindow, SW_SHOW);
    SetForegroundWindow(g_mainWindow);
    SetFocus(g_mainWindow);

    // Initialize D3D
    g_d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (g_d3d == NULL)
    {
        ShowD3DErrorMessage("Initializing D3D", 0);
        return 1;
    }

    D3DPRESENT_PARAMETERS presentParams;
    ZeroMemory(&presentParams, sizeof(presentParams));
    presentParams.Windowed = TRUE;
    presentParams.SwapEffect = D3DSWAPEFFECT_COPY;
#if 0
    presentParams.BackBufferWidth = 300;
    presentParams.BackBufferHeight = 300;
    presentParams.BackBufferCount = 1;
    presentParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    presentParams.Windowed = TRUE;
#endif
    
    HRESULT hr = g_d3d->CreateDevice(D3DADAPTER_DEFAULT,
                                     D3DDEVTYPE_HAL,
                                     g_mainWindow,
                                     D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                     &presentParams,
                                     &g_d3dDev);
    if (FAILED(hr))
    {
        ShowD3DErrorMessage("Creating D3D device", hr);
        //return 1;
    }

    string inputFilename(lpCmdLine);
    string outputFilename(inputFilename, 0, inputFilename.rfind('.'));
    outputFilename += ".cmod";

    ID3DXMesh* mesh = NULL;
    ID3DXBuffer* adjacency = NULL;
    ID3DXBuffer* materialBuf = NULL;
    ID3DXBuffer* effects = NULL;
    DWORD numMaterials;
    
    hr = D3DXLoadMeshFromX(inputFilename.c_str(),
                           0,
                           g_d3dDev,
                           &adjacency,
                           &materialBuf,
                           &effects,
                           &numMaterials,
                           &mesh);
    if (FAILED(hr))
    {
        ShowD3DErrorMessage("Loading mesh from X file", hr);
        return 1;
    }


    DWORD numVertices = mesh->GetNumVertices();
    DWORD numFaces = mesh->GetNumFaces();

    cout << "vertices: " << numVertices << '\n';
    cout << "faces: " << numFaces << '\n';

    cout << "adjacency buffer size: " << adjacency->GetBufferSize() << '\n';

    ofstream meshfile(outputFilename.c_str());

    // Output the header
    meshfile << "#celmodel__ascii\n\n";

    cout << "numMaterials=" << numMaterials << '\n';
    D3DXMATERIAL* materials = reinterpret_cast<D3DXMATERIAL*>(materialBuf->GetBufferPointer());
    for (DWORD mat = 0; mat < numMaterials; mat++)
    {
        meshfile << "material\n";
        meshfile << "diffuse " << materials[mat].MatD3D.Diffuse << '\n';
        //meshfile << "emissive " << materials[mat].MatD3D.Emissive << '\n';
        meshfile << "specular " << materials[mat].MatD3D.Specular << '\n';
        meshfile << "specpower " << materials[mat].MatD3D.Power << '\n';
        meshfile << "opacity " << materials[mat].MatD3D.Diffuse.a << '\n';
        meshfile << "end_material\n\n";
    }

    // Vertex format
    D3DVERTEXELEMENT9 declElements[MAX_FVF_DECL_SIZE];
    hr = mesh->GetDeclaration(declElements);
    if (FAILED(hr))
    {
        ShowD3DErrorMessage("Checking vertex declaration", hr);
        return 1;
    }

    DWORD stride = D3DXGetDeclVertexSize(declElements, 0);

    VertexAttribute vertexMap[VertexAttribute::MaxAttribute];
    CreateVertexAttributeMap(declElements, vertexMap);

    meshfile << "mesh\n\n";

    DumpVertexDescription(vertexMap, meshfile);

    ID3DXMesh* optMesh = NULL;
    ID3DXBuffer* vertexRemap = NULL;
    DWORD* faceRemap = new DWORD[numFaces];
    DWORD* optAdjacency = new DWORD[numFaces * 3];
    hr = mesh->Optimize(D3DXMESHOPT_COMPACT | D3DXMESHOPT_STRIPREORDER,
                        //D3DXMESHOPT_VERTEXCACHE |
                        reinterpret_cast<DWORD*>(adjacency->GetBufferPointer()),
                        optAdjacency,
                        faceRemap,
                        &vertexRemap,
                        &optMesh);
    if (FAILED(hr))
    {
        ShowD3DErrorMessage("Optimize failed: ", hr);
        return 1;
    }
    
    // Attribute table
    DWORD attribTableSize = 0;
    hr = optMesh->GetAttributeTable(NULL, &attribTableSize);
    if (FAILED(hr))
    {
        ShowD3DErrorMessage("Querying attribute table size", hr);
        return 1;
    }

    D3DXATTRIBUTERANGE* attribTable = NULL;
    if (attribTableSize > 0)
    {
        attribTable = new D3DXATTRIBUTERANGE[attribTableSize];
        hr = optMesh->GetAttributeTable(attribTable, &attribTableSize);
        if (FAILED(hr))
        {
            ShowD3DErrorMessage("Getting attribute table", hr);
            return 1;
        }
    }

    cout << "Attribute table size: " << attribTableSize << '\n';
    if (attribTableSize == 1)
    {
        cout << "Attribute id: " << attribTable[0].AttribId << '\n';
    }

    if (!DumpMeshVertices(optMesh, vertexMap, stride, meshfile))
        return 1;
    
    // output the indices
    for (DWORD attr = 0; attr < attribTableSize; attr++)
    {
        StripifyMeshSubset(optMesh, attr, meshfile);
    }
    meshfile << "\nend_mesh\n";

#if 0
    IDirect3DIndexBuffer9* indices = NULL;
    hr = mesh->GetIndexBuffer(&indices);
#endif

#if 0
    // No message loop required for this app
    MSG msg;
    GetMessage(&msg, NULL, 0u, 0u);
    while (msg.message != WM_QUIT)
    {
        GetMessage(&msg, NULL, 0u, 0u);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif

    return 0;
}


int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: xtocmod <meshfile.x>\n";
        return 1;
    }

    ID3DXMesh* mesh = NULL;
    ID3DXBuffer* adjacency = NULL;
    ID3DXBuffer* materials = NULL;
    ID3DXBuffer* effects = NULL;
    
    return 0;
}


char* D3DErrorString(HRESULT hr)
{
    switch (hr)
    {
    case D3DERR_WRONGTEXTUREFORMAT:
        return "D3DERR_WRONGTEXTUREFORMAT";
    case D3DERR_UNSUPPORTEDCOLOROPERATION:
        return "D3DERR_UNSUPPORTEDCOLOROPERATION";
    case D3DERR_UNSUPPORTEDCOLORARG:
        return "D3DERR_UNSUPPORTEDCOLORARG";
    case D3DERR_UNSUPPORTEDALPHAOPERATION:
        return "D3DERR_UNSUPPORTEDALPHAOPERATION";
    case D3DERR_UNSUPPORTEDALPHAARG:
        return "D3DERR_UNSUPPORTEDALPHAARG";
    case D3DERR_TOOMANYOPERATIONS:
        return "D3DERR_TOOMANYOPERATIONS";
    case D3DERR_CONFLICTINGTEXTUREFILTER:
        return "D3DERR_CONFLICTINGTEXTUREFILTER";
    case D3DERR_UNSUPPORTEDFACTORVALUE:
        return "D3DERR_UNSUPPORTEDFACTORVALUE";
    case D3DERR_CONFLICTINGRENDERSTATE:
        return "D3DERR_CONFLICTINGRENDERSTATE";
    case D3DERR_UNSUPPORTEDTEXTUREFILTER:
        return "D3DERR_UNSUPPORTEDTEXTUREFILTER";
    case D3DERR_CONFLICTINGTEXTUREPALETTE:
        return "D3DERR_CONFLICTINGTEXTUREPALETTE";
    case D3DERR_DRIVERINTERNALERROR:
        return "D3DERR_DRIVERINTERNALERROR";
    case D3DERR_NOTFOUND:
        return "D3DERR_NOTFOUND";
    case D3DERR_MOREDATA:
        return "D3DERR_MOREDATA";
    case D3DERR_DEVICELOST:
        return "D3DERR_DEVICELOST";
    case D3DERR_DEVICENOTRESET:
        return "D3DERR_DEVICENOTRESET";
    case D3DERR_NOTAVAILABLE:
        return "D3DERR_NOTAVAILABLE";
    case D3DERR_OUTOFVIDEOMEMORY:
        return "D3DERR_OUTOFVIDEOMEMORY";
    case D3DERR_INVALIDDEVICE:
        return "D3DERR_INVALIDDEVICE";
    case D3DERR_INVALIDCALL:
        return "D3DERR_INVALIDCALL";
    case D3DERR_DRIVERINVALIDCALL:
        return "D3DERR_DRIVERINVALIDCALL";
    case D3DERR_WASSTILLDRAWING:
        return "D3DERR_WASSTILLDRAWING";
    case D3DOK_NOAUTOGEN:
        return "D3DOK_NOAUTOGEN";

    case D3DXERR_CANNOTMODIFYINDEXBUFFER:
        return "D3DXERR_CANNOTMODIFYINDEXBUFFER";
    case D3DXERR_INVALIDMESH:
        return "D3DXERR_INVALIDMESH";
    case D3DXERR_CANNOTATTRSORT:
        return "D3DXERR_CANNOTATTRSORT";
    case D3DXERR_SKINNINGNOTSUPPORTED:
        return "D3DXERR_SKINNINGNOTSUPPORTED";
    case D3DXERR_TOOMANYINFLUENCES:
        return "D3DXERR_TOOMANYINFLUENCES";
    case D3DXERR_INVALIDDATA:
        return "D3DXERR_INVALIDDATA";
    case D3DXERR_LOADEDMESHASNODATA:
        return "D3DXERR_LOADEDMESHASNODATA";
    case D3DXERR_DUPLICATENAMEDFRAGMENT:
        return "D3DXERR_DUPLICATENAMEDFRAGMENT";

    default:
        return "Unkown D3D Error";
    }
}


void ShowD3DErrorMessage(char* info, HRESULT hr)
{
    char buf[1024];
    
    sprintf(buf, "%s - %s", info, D3DErrorString(hr));
    MessageBox(g_mainWindow,
               buf, "Fatal Error",
               MB_OK | MB_ICONERROR);
}

