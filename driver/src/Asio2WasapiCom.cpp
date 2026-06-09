#include "Asio2WasapiDriver.h"

#include <windows.h>
#include <objbase.h>
#include <unknwn.h>

#include <atomic>

class Asio2WasapiClassFactory final : public IClassFactory
{
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (!ppvObject)
            return E_POINTER;

        *ppvObject = nullptr;

        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
        {
            *ppvObject = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return ++refCount_;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG count = --refCount_;

        if (count == 0)
            delete this;

        return count;
    }

    HRESULT STDMETHODCALLTYPE CreateInstance(
        IUnknown* pUnkOuter,
        REFIID riid,
        void** ppvObject) override
    {
        if (!ppvObject)
            return E_POINTER;

        *ppvObject = nullptr;

        if (pUnkOuter)
            return CLASS_E_NOAGGREGATION;

        auto* driver = new Asio2WasapiDriver();

        const HRESULT result = driver->QueryInterface(riid, ppvObject);
        driver->Release();

        return result;
    }

    HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override
    {
        (void)lock;
        return S_OK;
    }

private:
    std::atomic<ULONG> refCount_{1};
};

STDAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID* ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (!IsEqualCLSID(rclsid, CLSID_Asio2WasapiVirtualAsio))
        return CLASS_E_CLASSNOTAVAILABLE;

    auto* factory = new Asio2WasapiClassFactory();

    const HRESULT result = factory->QueryInterface(riid, reinterpret_cast<void**>(ppv));
    factory->Release();

    return result;
}

STDAPI DllCanUnloadNow()
{
    return S_FALSE;
}