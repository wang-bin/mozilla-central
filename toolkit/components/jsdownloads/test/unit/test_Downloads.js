/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

/**
 * Tests the functions located directly in the "Downloads" object.
 */

"use strict";

////////////////////////////////////////////////////////////////////////////////
//// Tests

/**
 * Tests that the createDownload function exists and can be called.  More
 * detailed tests are implemented separately for the DownloadCore module.
 */
add_task(function test_createDownload()
{
  // Creates a simple Download object without starting the download.
  yield Downloads.createDownload({
    source: { url: "about:blank" },
    target: { path: getTempFile(TEST_TARGET_FILE_NAME).path },
    saver: { type: "copy" },
  });
});

/**
 * Tests createDownload for private download.
 */
add_task(function test_createDownload_private()
{
  let download = yield Downloads.createDownload({
    source: { url: "about:blank", isPrivate: true },
    target: { path: getTempFile(TEST_TARGET_FILE_NAME).path },
    saver: { type: "copy" }
  });
  do_check_true(download.source.isPrivate);
});

/**
 * Tests createDownload for normal (public) download.
 */
add_task(function test_createDownload_public()
{
  let tempPath = getTempFile(TEST_TARGET_FILE_NAME).path;
  let download = yield Downloads.createDownload({
    source: { url: "about:blank", isPrivate: false },
    target: { path: tempPath },
    saver: { type: "copy" }
  });
  do_check_false(download.source.isPrivate);

  download = yield Downloads.createDownload({
    source: { url: "about:blank" },
    target: { path: tempPath },
    saver: { type: "copy" }
  });
  do_check_false(download.source.isPrivate);
});

/**
 * Tests simpleDownload with nsIURI and nsIFile as arguments.
 */
add_task(function test_simpleDownload_uri_file_arguments()
{
  let targetFile = getTempFile(TEST_TARGET_FILE_NAME);
  yield Downloads.simpleDownload(NetUtil.newURI(httpUrl("source.txt")),
                                 targetFile);
  yield promiseVerifyContents(targetFile.path, TEST_DATA_SHORT);
});

/**
 * Tests simpleDownload with DownloadSource and DownloadTarget as arguments.
 */
add_task(function test_simpleDownload_object_arguments()
{
  let targetPath = getTempFile(TEST_TARGET_FILE_NAME).path;
  yield Downloads.simpleDownload({ url: httpUrl("source.txt") },
                                 { path: targetPath });
  yield promiseVerifyContents(targetPath, TEST_DATA_SHORT);
});

/**
 * Tests simpleDownload with string arguments.
 */
add_task(function test_simpleDownload_string_arguments()
{
  let targetPath = getTempFile(TEST_TARGET_FILE_NAME).path;
  yield Downloads.simpleDownload(httpUrl("source.txt"),
                                 targetPath);
  yield promiseVerifyContents(targetPath, TEST_DATA_SHORT);

  targetPath = getTempFile(TEST_TARGET_FILE_NAME).path;
  yield Downloads.simpleDownload(new String(httpUrl("source.txt")),
                                 new String(targetPath));
  yield promiseVerifyContents(targetPath, TEST_DATA_SHORT);
});

/**
 * Tests that the getPublicDownloadList function returns the same list when
 * called multiple times.  More detailed tests are implemented separately for
 * the DownloadList module.
 */
add_task(function test_getPublicDownloadList()
{
  let downloadListOne = yield Downloads.getPublicDownloadList();
  let downloadListTwo = yield Downloads.getPublicDownloadList();

  do_check_eq(downloadListOne, downloadListTwo);
});

/**
 * Tests that the getPrivateDownloadList function returns the same list when
 * called multiple times.  More detailed tests are implemented separately for
 * the DownloadList module.
 */
add_task(function test_getPrivateDownloadList()
{
  let downloadListOne = yield Downloads.getPrivateDownloadList();
  let downloadListTwo = yield Downloads.getPrivateDownloadList();

  do_check_eq(downloadListOne, downloadListTwo);
});

/**
 * Tests that the getPublicDownloadList and getPrivateDownloadList function
 * and returns the different list.  More detailed tests are implemented
 * separately for the DownloadList module.
 */
add_task(function test_public_and_private_lists_differ()
{
  let publicDownloadList = yield Downloads.getPublicDownloadList();
  let privateDownloadList = yield Downloads.getPrivateDownloadList();

  do_check_neq(publicDownloadList, privateDownloadList);
});

/**
 * Tests that the getSystemDownloadsDirectory returns a valid nsFile
 * download directory object.
 */
add_task(function test_getSystemDownloadsDirectory()
{
  let downloadDir = yield Downloads.getSystemDownloadsDirectory();
  do_check_true(downloadDir instanceof Ci.nsIFile);
});

/**
 * Tests that the getUserDownloadsDirectory returns a valid nsFile
 * download directory object.
 */
add_task(function test_getUserDownloadsDirectory()
{
  let downloadDir = yield Downloads.getUserDownloadsDirectory();
  do_check_true(downloadDir instanceof Ci.nsIFile);
});

/**
 * Tests that the getTemporaryDownloadsDirectory returns a valid nsFile
 * download directory object.
 */
add_task(function test_getTemporaryDownloadsDirectory()
{
  let downloadDir = yield Downloads.getTemporaryDownloadsDirectory();
  do_check_true(downloadDir instanceof Ci.nsIFile);
});
