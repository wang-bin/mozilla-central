/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 sw=2 et tw=78: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsHistory.h"
#include "mozilla/dom/HistoryBinding.h"
#include "nsCOMPtr.h"
#include "nsPIDOMWindow.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIDocShell.h"
#include "nsIWebNavigation.h"
#include "nsIHistoryEntry.h"
#include "nsIURI.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsReadableUtils.h"
#include "nsContentUtils.h"
#include "nsISHistoryInternal.h"
#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::dom;

static const char* sAllowPushStatePrefStr =
  "browser.history.allowPushState";
static const char* sAllowReplaceStatePrefStr =
  "browser.history.allowReplaceState";

//
//  History class implementation
//
NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(nsHistory)
NS_IMPL_CYCLE_COLLECTING_ADDREF(nsHistory)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsHistory)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsHistory)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHistory) // Empty, needed for extension compat
NS_INTERFACE_MAP_END

nsHistory::nsHistory(nsPIDOMWindow* aInnerWindow)
  : mInnerWindow(do_GetWeakReference(aInnerWindow))
{
  SetIsDOMBinding();
}

nsHistory::~nsHistory()
{
}

nsPIDOMWindow*
nsHistory::GetParentObject() const
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  return win;
}

JSObject*
nsHistory::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return HistoryBinding::Wrap(aCx, aScope, this);
}

uint32_t
nsHistory::GetLength(ErrorResult& aRv) const
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win || !nsContentUtils::CanCallerAccess(win->GetOuterWindow())) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);

    return 0;
  }

  // Get session History from docshell
  nsCOMPtr<nsISHistory> sHistory = GetSessionHistory();
  if (!sHistory) {
    aRv.Throw(NS_ERROR_FAILURE);

    return 0;
  }

  int32_t len;
  nsresult rv = sHistory->GetCount(&len);

  if (NS_FAILED(rv)) {
    aRv.Throw(rv);

    return 0;
  }

  return len >= 0 ? len : 0;
}

JS::Value
nsHistory::GetState(JSContext* aCx, ErrorResult& aRv) const
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win) {
    aRv.Throw(NS_ERROR_NOT_AVAILABLE);

    return JS::UndefinedValue();
  }

  if (!nsContentUtils::CanCallerAccess(win->GetOuterWindow())) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);

    return JS::UndefinedValue();
  }

  nsCOMPtr<nsIDocument> doc =
    do_QueryInterface(win->GetExtantDoc());
  if (!doc) {
    aRv.Throw(NS_ERROR_NOT_AVAILABLE);

    return JS::UndefinedValue();
  }

  nsCOMPtr<nsIVariant> variant;
  doc->GetStateObject(getter_AddRefs(variant));

  if (variant) {
    JS::Rooted<JS::Value> jsData(aCx);
    aRv = variant->GetAsJSVal(jsData.address());

    if (aRv.Failed()) {
      return JS::UndefinedValue();
    }

    if (!JS_WrapValue(aCx, jsData.address())) {
      aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return JS::UndefinedValue();
    }

    return jsData;
  }

  return JS::UndefinedValue();
}

void
nsHistory::Go(int32_t aDelta, ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win || !nsContentUtils::CanCallerAccess(win->GetOuterWindow())) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);

    return;
  }

  if (!aDelta) {
    nsCOMPtr<nsPIDOMWindow> window(do_GetInterface(GetDocShell()));

    if (window && window->IsHandlingResizeEvent()) {
      // history.go(0) (aka location.reload()) was called on a window
      // that is handling a resize event. Sites do this since Netscape
      // 4.x needed it, but we don't, and it's a horrible experience
      // for nothing.  In stead of reloading the page, just clear
      // style data and reflow the page since some sites may use this
      // trick to work around gecko reflow bugs, and this should have
      // the same effect.

      nsCOMPtr<nsIDocument> doc = window->GetExtantDoc();

      nsIPresShell *shell;
      nsPresContext *pcx;
      if (doc && (shell = doc->GetShell()) && (pcx = shell->GetPresContext())) {
        pcx->RebuildAllStyleData(NS_STYLE_HINT_REFLOW);
      }

      return;
    }
  }

  nsCOMPtr<nsISHistory> session_history = GetSessionHistory();
  nsCOMPtr<nsIWebNavigation> webnav(do_QueryInterface(session_history));
  if (!webnav) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  int32_t curIndex = -1;
  int32_t len = 0;
  session_history->GetIndex(&curIndex);
  session_history->GetCount(&len);

  int32_t index = curIndex + aDelta;
  if (index > -1 && index < len)
    webnav->GotoIndex(index);

  // Ignore the return value from GotoIndex(), since returning errors
  // from GotoIndex() can lead to exceptions and a possible leak
  // of history length
}

void
nsHistory::Back(ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win || !nsContentUtils::CanCallerAccess(win->GetOuterWindow())) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);

    return;
  }

  nsCOMPtr<nsISHistory> sHistory = GetSessionHistory();
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(sHistory));
  if (!webNav) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  webNav->GoBack();
}

void
nsHistory::Forward(ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win || !nsContentUtils::CanCallerAccess(win->GetOuterWindow())) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);

    return;
  }

  nsCOMPtr<nsISHistory> sHistory = GetSessionHistory();
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(sHistory));
  if (!webNav) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  webNav->GoForward();
}

void
nsHistory::PushState(JSContext* aCx, JS::Handle<JS::Value> aData,
                     const nsAString& aTitle, const nsAString& aUrl,
                     ErrorResult& aRv)
{
  PushOrReplaceState(aCx, aData, aTitle, aUrl, aRv, false);
}

void
nsHistory::ReplaceState(JSContext* aCx, JS::Handle<JS::Value> aData,
                        const nsAString& aTitle, const nsAString& aUrl,
                        ErrorResult& aRv)
{
  PushOrReplaceState(aCx, aData, aTitle, aUrl, aRv, true);
}

void
nsHistory::GetCurrent(nsString& aRetval, ErrorResult& aRv) const
{
  MOZ_ASSERT(nsContentUtils::IsCallerChrome());

  aRetval.Truncate();

  int32_t curIndex = 0;
  nsAutoCString curURL;
  nsCOMPtr<nsISHistory> sHistory = GetSessionHistory();
  if (!sHistory) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  // Get the current index at session History
  sHistory->GetIndex(&curIndex);
  nsCOMPtr<nsIHistoryEntry> curEntry;
  nsCOMPtr<nsIURI> uri;

  // Get the SH entry for the current index
  sHistory->GetEntryAtIndex(curIndex, false, getter_AddRefs(curEntry));
  if (!curEntry) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  // Get the URI for the current entry
  curEntry->GetURI(getter_AddRefs(uri));
  if (!uri) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  uri->GetSpec(curURL);
  CopyUTF8toUTF16(curURL, aRetval);
}

void
nsHistory::GetPrevious(nsString& aRetval, ErrorResult& aRv) const
{
  MOZ_ASSERT(nsContentUtils::IsCallerChrome());

  aRetval.Truncate();

  int32_t curIndex;
  nsAutoCString prevURL;
  nsCOMPtr<nsISHistory> sHistory = GetSessionHistory();
  if (!sHistory) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  // Get the current index at session History
  sHistory->GetIndex(&curIndex);
  nsCOMPtr<nsIHistoryEntry> prevEntry;
  nsCOMPtr<nsIURI> uri;

  // Get the previous SH entry
  sHistory->GetEntryAtIndex((curIndex - 1), false, getter_AddRefs(prevEntry));
  if (!prevEntry) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  // Get the URI for the previous entry
  prevEntry->GetURI(getter_AddRefs(uri));
  if (!uri) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  uri->GetSpec(prevURL);
  CopyUTF8toUTF16(prevURL, aRetval);
}

void
nsHistory::GetNext(nsString& aRetval, ErrorResult& aRv) const
{
  MOZ_ASSERT(nsContentUtils::IsCallerChrome());

  aRetval.Truncate();

  int32_t curIndex;
  nsAutoCString nextURL;
  nsCOMPtr<nsISHistory> sHistory = GetSessionHistory();
  if (!sHistory) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  // Get the current index at session History
  sHistory->GetIndex(&curIndex);
  nsCOMPtr<nsIHistoryEntry> nextEntry;
  nsCOMPtr<nsIURI> uri;

  // Get the next SH entry
  sHistory->GetEntryAtIndex((curIndex+1), false, getter_AddRefs(nextEntry));
  if (!nextEntry) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  // Get the URI for the next entry
  nextEntry->GetURI(getter_AddRefs(uri));

  if (!uri) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  uri->GetSpec(nextURL);
  CopyUTF8toUTF16(nextURL, aRetval);
}

void
nsHistory::Item(uint32_t aIndex, nsString& aRetval, ErrorResult& aRv)
{
  bool unused;
  IndexedGetter(aIndex, unused, aRetval, aRv);
}

void
nsHistory::IndexedGetter(uint32_t aIndex, bool &aFound, nsString& aRetval,
                         ErrorResult& aRv)
{
  aRetval.Truncate();
  aFound = false;

  if (!nsContentUtils::IsCallerChrome()) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);

    return;
  }

  nsCOMPtr<nsISHistory> session_history = GetSessionHistory();
  if (!session_history) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  nsCOMPtr<nsIHistoryEntry> sh_entry;
  nsCOMPtr<nsIURI> uri;

  aRv = session_history->GetEntryAtIndex(aIndex, false,
                                         getter_AddRefs(sh_entry));

  if (aRv.Failed()) {
    return;
  }

  if (sh_entry) {
    aRv = sh_entry->GetURI(getter_AddRefs(uri));
  }

  if (uri) {
    nsAutoCString urlCString;
    aRv = uri->GetSpec(urlCString);

    if (aRv.Failed()) {
      return;
    }

    CopyUTF8toUTF16(urlCString, aRetval);

    aFound = true;
  }
}

uint32_t
nsHistory::Length()
{
  if (!nsContentUtils::IsCallerChrome()) {
    return 0;
  }

  // Get session History from docshell
  nsCOMPtr<nsISHistory> sHistory = GetSessionHistory();
  if (!sHistory) {
    return 0;
  }

  int32_t len;
  nsresult rv = sHistory->GetCount(&len);

  if (NS_FAILED(rv) || len < 0) {
    return 0;
  }

  return len;
}

void
nsHistory::PushOrReplaceState(JSContext* aCx, JS::Value aData,
                              const nsAString& aTitle, const nsAString& aUrl,
                              ErrorResult& aRv, bool aReplace)
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win) {
    aRv.Throw(NS_ERROR_NOT_AVAILABLE);

    return;
  }

  if (!nsContentUtils::CanCallerAccess(win->GetOuterWindow())) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);

    return;
  }

  // Check that PushState hasn't been pref'ed off.
  if (!Preferences::GetBool(aReplace ? sAllowReplaceStatePrefStr :
                            sAllowPushStatePrefStr, false)) {
    return;
  }

  // AddState might run scripts, so we need to hold a strong reference to the
  // docShell here to keep it from going away.
  nsCOMPtr<nsIDocShell> docShell = win->GetDocShell();

  if (!docShell) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  // The "replace" argument tells the docshell to whether to add a new
  // history entry or modify the current one.

  aRv = docShell->AddState(aData, aTitle, aUrl, aReplace, aCx);
}

already_AddRefed<nsISHistory>
nsHistory::GetSessionHistory() const
{
  nsIDocShell *docShell = GetDocShell();
  NS_ENSURE_TRUE(docShell, nullptr);

  // Get the root DocShell from it
  nsCOMPtr<nsIDocShellTreeItem> root;
  docShell->GetSameTypeRootTreeItem(getter_AddRefs(root));
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(root));
  NS_ENSURE_TRUE(webNav, nullptr);

  nsCOMPtr<nsISHistory> shistory;

  // Get SH from nsIWebNavigation
  webNav->GetSessionHistory(getter_AddRefs(shistory));

  return shistory.forget();
}
