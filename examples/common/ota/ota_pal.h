/*
 * FreeRTOS version 202107.00-LTS
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */


/**
 * @brief Header file containing platform abstraction layer APIS for OTA update.
 */

#ifndef OTA_PAL_H
#define OTA_PAL_H


#include "jobs.h"
#include "job_parser.h"

/**
 * @ingroup ota_enum_types
 * @brief OTA Image states.
 *
 * After an OTA update image is received and authenticated, it is logically moved to
 * the Self Test state by the OTA agent pending final acceptance. After the image is
 * activated and tested by your user code, you should put it into either the Accepted
 * or Rejected state by calling @ref OTA_SetImageState ( OtaImageStateAccepted ) or
 * @ref OTA_SetImageState ( OtaImageStateRejected ). If the image is accepted, it becomes
 * the main firmware image to be booted from then on. If it is rejected, the image is
 * no longer valid and shall not be used, reverting to the last known good image.
 *
 * If you want to abort an active OTA transfer, you may do so by calling the API
 * @ref OTA_SetImageState ( OtaImageStateAborted ).
 */
typedef enum OtaImageState
{
    OtaImageStateUnknown = 0,  /*!< @brief The initial state of the OTA MCU Image. */
    OtaImageStateTesting = 1,  /*!< @brief The state of the OTA MCU Image post successful download and reboot. */
    OtaImageStateAccepted = 2, /*!< @brief The state of the OTA MCU Image post successful download and successful self_test. */
    OtaImageStateRejected = 3, /*!< @brief The state of the OTA MCU Image when the job has been rejected. */
    OtaImageStateAborted = 4,  /*!< @brief The state of the OTA MCU Image after a timeout publish to the stream request fails.
                                *   Also if the OTA MCU image is aborted in the middle of a stream. */
    OtaLastImageState = OtaImageStateAborted
} OtaImageState_t;

/**
 * @ingroup ota_enum_types
 * @brief OTA Platform Image State.
 *
 * The image state set by platform implementation.
 */
typedef enum OtaPalImageState
{
    OtaPalImageStateUnknown = 0,   /*!< @brief The initial state of the OTA PAL Image. */
    OtaPalImageStatePendingCommit, /*!< @brief OTA PAL Image awaiting update. */
    OtaPalImageStateValid,         /*!< @brief OTA PAL Image is valid. */
    OtaPalImageStateInvalid        /*!< @brief OTA PAL Image is invalid. */
} OtaPalImageState_t;

/**
 * @ingroup ota_enum_types
 * @brief OTA Platform Image State.
 *
 * The image state set by platform implementation.
 */
typedef enum OtaPalJobDocProcessingResult
{
    OtaPalJobDocFileCreated = 0,
    OtaPalJobDocFileCreateFailed,
    OtaPalNewImageBooted,
    OtaPalNewImageBootFailed,
    OtaPalJobDocProcessingStateInvalid
} OtaPalJobDocProcessingResult_t;

typedef enum OtaPalStatus
{
    OtaPalSuccess = 0,
    OtaPalUninitialized,
    OtaPalOutOfMemory,
    OtaPalNullFileContext,
    OtaPalSignatureCheckFailed,
    OtaPalRxFileCreateFailed,
    OtaPalRxFileTooLarge,
    OtaPalBootInfoCreateFailed,
    OtaPalBadSignerCert,
    OtaPalBadImageState,
    OtaPalAbortFailed,
    OtaPalRejectFailed,
    OtaPalCommitFailed,
    OtaPalActivateFailed,
    OtaPalFileAbort,
    OtaPalFileClose
} OtaPalStatus_t;

/**
 * @brief Abort an OTA transfer.
 *
 * Aborts access to an existing open file represented by the OTA file context pFileContext. This is
 * only valid for jobs that started successfully.
 *
 * @note The input OtaFileContext_t pFileContext is checked for NULL by the OTA agent before this
 * function is called.
 *
 * This function may be called before the file is opened, so the file pointer pFileContext->fileHandle
 * may be NULL when this function is called.
 *
 * @param[in] pFileContext OTA file context information.
 *
 * @return The OtaPalStatus_t error code is a combination of the main OTA PAL interface error and
 *         the MCU specific sub error code. See ota_platform_interface.h for the OtaPalMainStatus_t
 *         error codes and your specific PAL implementation for the sub error code.
 *
 * Major error codes returned are:
 *
 *   OtaPalSuccess: Aborting access to the open file was successful.
 *   OtaPalFileAbort: Aborting access to the open file context was unsuccessful.
 */
OtaPalStatus_t otaPal_Abort( AfrOtaJobDocumentFields_t * const pFileContext );

/**
 * @brief Create a new receive file.
 *
 * @note Opens the file indicated in the OTA file context in the MCU file system.
 *
 * @note The previous image may be present in the designated image download partition or file, so the
 * partition or file must be completely erased or overwritten in this routine.
 *
 * @note The input OtaFileContext_t pFileContext is checked for NULL by the OTA agent before this
 * function is called.
 * The device file path is a required field in the OTA job document, so pFileContext->pFilePath is
 * checked for NULL by the OTA agent before this function is called.
 *
 * @param[in] pFileContext OTA file context information.
 *
 * @return The OtaPalStatus_t error code is a combination of the main OTA PAL interface error and
 *         the MCU specific sub error code. See ota_platform_interface.h for the OtaPalMainStatus_t
 *         error codes and your specific PAL implementation for the sub error code.
 *
 * Major error codes returned are:
 *
 *   OtaPalSuccess: File creation was successful.
 *   OtaPalRxFileTooLarge: The OTA receive file is too big for the platform to support.
 *   OtaPalBootInfoCreateFailed: The bootloader information file creation failed.
 *   OtaPalRxFileCreateFailed: Returned for other errors creating the file in the device's
 *                             non-volatile memory. If this error is returned, then the sub error
 *                             should be set to the appropriate platform specific value.
 */
OtaPalJobDocProcessingResult_t otaPal_CreateFileForRx( AfrOtaJobDocumentFields_t * const pFileContext );

/**
 * @brief Authenticate and close the underlying receive file in the specified OTA context.
 *
 * @note The input OtaFileContext_t pFileContext is checked for NULL by the OTA agent before this
 * function is called. This function is called only at the end of block ingestion.
 * otaPAL_CreateFileForRx() must succeed before this function is reached, so
 * pFileContext->fileHandle(or pFileContext->pFile) is never NULL.
 * The file signature key is required job document field in the OTA Agent, so pFileContext->pSignature will
 * never be NULL.
 *
 * If the signature verification fails, file close should still be attempted.
 *
 * @param[in] pFileContext OTA file context information.
 *
 * @return The OtaPalStatus_t error code is a combination of the main OTA PAL interface error and
 *         the MCU specific sub error code. See ota_platform_interface.h for the OtaPalMainStatus_t
 *         error codes and your specific PAL implementation for the sub error code.
 *
 * Major error codes returned are:
 *
 *   OtaPalSuccess on success.
 *   OtaPalSignatureCheckFailed: The signature check failed for the specified file.
 *   OtaPalBadSignerCert: The signer certificate was not readable or zero length.
 *   OtaPalFileClose: Error in low level file close.
 */
OtaPalStatus_t otaPal_CloseFile( AfrOtaJobDocumentFields_t * const pFileContext );

/**
 * @brief Write a block of data to the specified file at the given offset.
 *
 * @note The input OtaFileContext_t pFileContext is checked for NULL by the OTA agent before this
 * function is called.
 * The file pointer/handle pFileContext->pFile, is checked for NULL by the OTA agent before this
 * function is called.
 * pData is checked for NULL by the OTA agent before this function is called.
 * blockSize is validated for range by the OTA agent before this function is called.
 * offset is validated by the OTA agent before this function is called.
 *
 * @param[in] pFileContext OTA file context information.
 * @param[in] ulOffset Byte offset to write to from the beginning of the file.
 * @param[in] pData Pointer to the byte array of data to write.
 * @param[in] ulBlockSize The number of bytes to write.
 *
 * @return The number of bytes written successfully, or a negative error code from the platform
 * abstraction layer.
 */
int16_t otaPal_WriteBlock( AfrOtaJobDocumentFields_t * const pFileContext,
                           uint32_t ulOffset,
                           uint8_t * const pcData,
                           uint32_t ulBlockSize );

/**
 * @brief Activate the newest MCU image received via OTA.
 *
 * This function shall take necessary actions to activate the newest MCU
 * firmware received via OTA. It is typically just a reset of the device.
 *
 * @note This function SHOULD NOT return. If it does, the platform does not support
 * an automatic reset or an error occurred.
 *
 * @param[in] pFileContext OTA file context information.
 *
 * @return The OtaPalStatus_t error code is a combination of the main OTA PAL interface error and
 *         the MCU specific sub error code. See ota_platform_interface.h for the OtaPalMainStatus_t
 *         error codes and your specific PAL implementation for the sub error code.
 *
 * Major error codes returned are:
 *
 *   OtaPalSuccess on success.
 *   OtaPalActivateFailed: The activation of the new OTA image failed.
 */
OtaPalStatus_t otaPal_ActivateNewImage( AfrOtaJobDocumentFields_t * const pFileContext );

/**
 * @brief Attempt to set the state of the OTA update image.
 *
 * Take required actions on the platform to Accept/Reject the OTA update image (or bundle).
 * Refer to the PAL implementation to determine what happens on your platform.
 *
 * @param[in] pFileContext File context of type OtaFileContext_t.
 * @param[in] eState The desired state of the OTA update image.
 *
 * @return The OtaPalStatus_t error code is a combination of the main OTA PAL interface error and
 *         the MCU specific sub error code. See ota_platform_interface.h for the OtaPalMainStatus_t
 *         error codes and your specific PAL implementation for the sub error code.
 *
 * Major error codes returned are:
 *
 *   OtaPalSuccess on success.
 *   OtaPalBadImageState: if you specify an invalid OtaImageState_t. No sub error code.
 *   OtaPalAbortFailed: failed to roll back the update image as requested by OtaImageStateAborted.
 *   OtaPalRejectFailed: failed to roll back the update image as requested by OtaImageStateRejected.
 *   OtaPalCommitFailed: failed to make the update image permanent as requested by OtaImageStateAccepted.
 */
OtaPalStatus_t otaPal_SetPlatformImageState( AfrOtaJobDocumentFields_t * const pFileContext,
                                             OtaImageState_t eState );

/**
 * @brief Get the state of the OTA update image.
 *
 * We read this at OTA_Init time and when the latest OTA job reports itself in self
 * test. If the update image is in the "pending commit" state, we start a self test
 * timer to assure that we can successfully connect to the OTA services and accept
 * the OTA update image within a reasonable amount of time (user configurable). If
 * we don't satisfy that requirement, we assume there is something wrong with the
 * firmware and automatically reset the device, causing it to roll back to the
 * previously known working code.
 *
 * If the update image state is not in "pending commit," the self test timer is
 * not started.
 *
 * @param[in] pFileContext File context of type OtaFileContext_t.
 *
 * @return An OtaPalImageState_t. One of the following:
 *   OtaPalImageStatePendingCommit (the new firmware image is in the self test phase)
 *   OtaPalImageStateValid         (the new firmware image is already committed)
 *   OtaPalImageStateInvalid       (the new firmware image is invalid or non-existent)
 *
 *   NOTE: OtaPalImageStateUnknown should NEVER be returned and indicates an implementation error.
 */
OtaPalImageState_t otaPal_GetPlatformImageState( AfrOtaJobDocumentFields_t * const pFileContext );

/**
 * @brief Reset the device.
 *
 * This function shall reset the MCU and cause a reboot of the system.
 *
 * @note This function SHOULD NOT return. If it does, the platform does not support
 * an automatic reset or an error occurred.
 *
 * @param[in] pFileContext OTA file context information.
 *
 * @return The OtaPalStatus_t error code is a combination of the main OTA PAL interface error and
 *         the MCU specific sub error code. See ota_platform_interface.h for the OtaPalMainStatus_t
 *         error codes and your specific PAL implementation for the sub error code.
 */
OtaPalStatus_t otaPal_ResetDevice( AfrOtaJobDocumentFields_t * const pFileContext );

#endif /* ifndef OTA_PAL_H_ */
