/*! \file modules.h
 * \brief Module support
 *
 * $Id$
 *
 * To support loadable modules, we implement a poor man's COM. There is no
 * support for appartments, multiple threads, out of process servers, remote
 * servers, registry, or marshalling.  There is no RPC or sockets, and
 * most-likely, no opportunitity to use any existing RPC tools for building
 * interfaces either.
 */

typedef int MUX_RESULT;

#define MUX_S_FALSE              (0)
#define MUX_S_OK                 (1)
#define MUX_E_FAIL              (-1)
#define MUX_E_OUTOFMEMORY       (-2)
#define MUX_E_CLASSNOTAVAILABLE (-3)
#define MUX_E_NOINTERFACE       (-4)

#define MUX_FAILED(x)    ((MUX_RESULT)(x) < 0)
#define MUX_SUCCEEDED(x) (0 <= (MUX_RESULT)(x))

#ifdef WIN32
const UINT64 mux_IID_IUnknown      = 0x0000000100000010i64;
const UINT64 mux_IID_IClassFactory = 0x0000000100000011i64;
#else
const UINT64 mux_IID_IUnknown      = 0x0000000100000010ull;
const UINT64 mux_IID_IClassFactory = 0x0000000100000011ull;
#endif

#define interface class

interface mux_IUnknown
{
public:
    virtual MUX_RESULT QueryInterface(UINT64 iid, void **ppv) = 0;
    virtual UINT32     AddRef(void) = 0;
    virtual UINT32     Release(void) = 0;
};

interface mux_IClassFactory : public mux_IUnknown
{
public:
    virtual MUX_RESULT CreateInstance(UINT64 iid, void **ppv) = 0;
    virtual MUX_RESULT LockServer(bool bLock) = 0;
};

MUX_RESULT mux_CreateInstance(UINT64 cid, UINT64 iid, void **ppv);
