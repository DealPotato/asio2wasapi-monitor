#include "Asio2WasapiDriver.h"
#include "DebugLog.h"

#include <windows.h>
#include <objbase.h>
#include <unknwn.h>

#include <atomic>
#include <new>

const CLSID CLSID_Asio2WasapiVirtualAsio =
{
    0x85b9bdb2,
    0x2f44,
    0x4d13,
    { 0x9c, 0x7a, 0x2f, 0x28, 0x63, 0xa0, 0xd1, 0xd0 }
};

namespace
{
    std::atomic<long> g_serverLocks{ 0 };

    class Asio2WasapiClassFactory final : public IClassFactory
    {
    public:
        HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID riid,
            void** ppvObject) override
        {
            if (!ppvObject)
                return E_POINTER;

            *ppvObject = nullptr;

            if (IsEqualIID(riid, IID_IUnknown) ||
                IsEqualIID(riid, IID_IClassFactory))
            {
                *ppvObject = static_cast<IClassFactory*>(this);
                AddRef();
                return S_OK;
            }

            return E_NOINTERFACE;
        }

        ULONG STDMETHODCALLTYPE AddRef() override
        {
            return refCount_.fetch_add(1, std::memory_order_relaxed) + 1;
        }

        ULONG STDMETHODCALLTYPE Release() override
        {
            const ULONG count =
                refCount_.fetch_sub(1, std::memory_order_acq_rel) - 1;

            if (count == 0)
                delete this;

            return count;
        }

        HRESULT STDMETHODCALLTYPE CreateInstance(
            IUnknown* pUnkOuter,
            REFIID riid,
            void** ppvObject) override
        {
            debugLog("[ASIO2WASAPI] ClassFactory::CreateInstance called\n");

            if (!ppvObject)
                return E_POINTER;

            *ppvObject = nullptr;

            if (pUnkOuter)
                return CLASS_E_NOAGGREGATION;

            auto* driver = new (std::nothrow) Asio2WasapiDriver();

            if (!driver)
                return E_OUTOFMEMORY;

            const HRESULT result = driver->QueryInterface(riid, ppvObject);
            driver->Release();

            return result;
        }

        HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override
        {
            if (lock)
                g_serverLocks.fetch_add(1, std::memory_order_relaxed);
            else
                g_serverLocks.fetch_sub(1, std::memory_order_relaxed);

            return S_OK;
        }

    private:
        std::atomic<ULONG> refCount_{ 1 };
    };
}

STDAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID* ppvObject)
{
    debugLog("[ASIO2WASAPI] DllGetClassObject called\n");

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = nullptr;

    if (!IsEqualCLSID(rclsid, CLSID_Asio2WasapiVirtualAsio))
        return CLASS_E_CLASSNOTAVAILABLE;

    auto* factory = new (std::nothrow) Asio2WasapiClassFactory();

    if (!factory)
        return E_OUTOFMEMORY;

    const HRESULT result = factory->QueryInterface(riid, ppvObject);
    factory->Release();

    return result;
}

STDAPI DllCanUnloadNow()
{
    debugLog("[ASIO2WASAPI] DllCanUnloadNow called\n");

    // Keep it loaded for now. Safer during early driver development.
    return S_FALSE;
}