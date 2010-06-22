#include "nsIClassInfoImpl.h"
#include "nsIProgrammingLanguage.h"

NS_IMETHODIMP_(nsrefcnt)
GenericClassInfo::AddRef()
{
  return 2;
}

NS_IMETHODIMP_(nsrefcnt)
GenericClassInfo::Release()
{
  return 1;
}

NS_IMPL_QUERY_INTERFACE1(GenericClassInfo, nsIClassInfo)

NS_IMETHODIMP
GenericClassInfo::GetInterfaces(PRUint32* countp, nsIID*** array)
{
  return mData->getinterfaces(countp, array);
}

NS_IMETHODIMP
GenericClassInfo::GetHelperForLanguage(PRUint32 language, nsISupports** helper)
{
  if (mData->getlanguagehelper)
    return mData->getlanguagehelper(language, helper);
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
GenericClassInfo::GetContractID(char** contractid)
{
  NS_ERROR("GetContractID not implemented");
  *contractid = NULL;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
GenericClassInfo::GetClassDescription(char** description)
{
  NS_ERROR("GetClassDescription not implemented");
  *description = NULL;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
GenericClassInfo::GetClassID(nsCID** classid)
{
  NS_ERROR("GetClassID not implemented");
  *classid = NULL;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
GenericClassInfo::GetImplementationLanguage(PRUint32* language)
{
  *language = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
GenericClassInfo::GetFlags(PRUint32* flags)
{
  *flags = mData->flags;
  return NS_OK;
}

NS_IMETHODIMP
GenericClassInfo::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  *aClassIDNoAlloc = mData->cid;
  return NS_OK;
}
