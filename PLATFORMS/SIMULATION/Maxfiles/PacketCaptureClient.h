/**\file */
#ifndef SLIC_DECLARATIONS_PacketCaptureClient_H
#define SLIC_DECLARATIONS_PacketCaptureClient_H
#include "MaxSLiCInterface.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PacketCaptureClient_USE_NETWORK_MANAGER (1)
#define PacketCaptureClient_PCIE_ALIGNMENT (16)
#define PacketCaptureClient_server_if_name ("QSFP_TOP_10G_PORT2")


/*----------------------------------------------------------------------------*/
/*---------------------- Interface enableRemoteCapture -----------------------*/
/*----------------------------------------------------------------------------*/




/**
 * \brief Basic static function for the interface 'enableRemoteCapture'.
 * 
 * \param [in] param_socketsALen Interface Parameter "socketsALen".
 * \param [in] param_socketsBLen Interface Parameter "socketsBLen".
 * \param [in] param_socketsA Interface Parameter array socketsA[] should be of size 2.
 * \param [in] param_socketsB Interface Parameter array socketsB[] should be of size 2.
 */
void PacketCaptureClient_enableRemoteCapture(
	uint8_t param_socketsALen,
	uint8_t param_socketsBLen,
	const uint8_t *param_socketsA,
	const uint8_t *param_socketsB);

/**
 * \brief Basic static non-blocking function for the interface 'enableRemoteCapture'.
 * 
 * Schedule to run on an engine and return immediately.
 * The status of the run can be checked either by ::max_wait or ::max_nowait;
 * note that one of these *must* be called, so that associated memory can be released.
 * 
 * 
 * \param [in] param_socketsALen Interface Parameter "socketsALen".
 * \param [in] param_socketsBLen Interface Parameter "socketsBLen".
 * \param [in] param_socketsA Interface Parameter array socketsA[] should be of size 2.
 * \param [in] param_socketsB Interface Parameter array socketsB[] should be of size 2.
 * \return A handle on the execution status, or NULL in case of error.
 */
max_run_t *PacketCaptureClient_enableRemoteCapture_nonblock(
	uint8_t param_socketsALen,
	uint8_t param_socketsBLen,
	const uint8_t *param_socketsA,
	const uint8_t *param_socketsB);

/**
 * \brief Advanced static interface, structure for the engine interface 'enableRemoteCapture'
 * 
 */
typedef struct { 
	uint8_t param_socketsALen; /**<  [in] Interface Parameter "socketsALen". */
	uint8_t param_socketsBLen; /**<  [in] Interface Parameter "socketsBLen". */
	const uint8_t *param_socketsA; /**<  [in] Interface Parameter array socketsA[] should be of size 2. */
	const uint8_t *param_socketsB; /**<  [in] Interface Parameter array socketsB[] should be of size 2. */
} PacketCaptureClient_enableRemoteCapture_actions_t;

/**
 * \brief Advanced static function for the interface 'enableRemoteCapture'.
 * 
 * \param [in] engine The engine on which the actions will be executed.
 * \param [in,out] interface_actions Actions to be executed.
 */
void PacketCaptureClient_enableRemoteCapture_run(
	max_engine_t *engine,
	PacketCaptureClient_enableRemoteCapture_actions_t *interface_actions);

/**
 * \brief Advanced static non-blocking function for the interface 'enableRemoteCapture'.
 *
 * Schedule the actions to run on the engine and return immediately.
 * The status of the run can be checked either by ::max_wait or ::max_nowait;
 * note that one of these *must* be called, so that associated memory can be released.
 *
 * 
 * \param [in] engine The engine on which the actions will be executed.
 * \param [in] interface_actions Actions to be executed.
 * \return A handle on the execution status of the actions, or NULL in case of error.
 */
max_run_t *PacketCaptureClient_enableRemoteCapture_run_nonblock(
	max_engine_t *engine,
	PacketCaptureClient_enableRemoteCapture_actions_t *interface_actions);

/**
 * \brief Group run advanced static function for the interface 'enableRemoteCapture'.
 * 
 * \param [in] group Group to use.
 * \param [in,out] interface_actions Actions to run.
 *
 * Run the actions on the first device available in the group.
 */
void PacketCaptureClient_enableRemoteCapture_run_group(max_group_t *group, PacketCaptureClient_enableRemoteCapture_actions_t *interface_actions);

/**
 * \brief Group run advanced static non-blocking function for the interface 'enableRemoteCapture'.
 * 
 *
 * Schedule the actions to run on the first device available in the group and return immediately.
 * The status of the run must be checked with ::max_wait. 
 * Note that use of ::max_nowait is prohibited with non-blocking running on groups:
 * see the ::max_run_group_nonblock documentation for more explanation.
 *
 * \param [in] group Group to use.
 * \param [in] interface_actions Actions to run.
 * \return A handle on the execution status of the actions, or NULL in case of error.
 */
max_run_t *PacketCaptureClient_enableRemoteCapture_run_group_nonblock(max_group_t *group, PacketCaptureClient_enableRemoteCapture_actions_t *interface_actions);

/**
 * \brief Array run advanced static function for the interface 'enableRemoteCapture'.
 * 
 * \param [in] engarray The array of devices to use.
 * \param [in,out] interface_actions The array of actions to run.
 *
 * Run the array of actions on the array of engines.  The length of interface_actions
 * must match the size of engarray.
 */
void PacketCaptureClient_enableRemoteCapture_run_array(max_engarray_t *engarray, PacketCaptureClient_enableRemoteCapture_actions_t *interface_actions[]);

/**
 * \brief Array run advanced static non-blocking function for the interface 'enableRemoteCapture'.
 * 
 *
 * Schedule to run the array of actions on the array of engines, and return immediately.
 * The length of interface_actions must match the size of engarray.
 * The status of the run can be checked either by ::max_wait or ::max_nowait;
 * note that one of these *must* be called, so that associated memory can be released.
 *
 * \param [in] engarray The array of devices to use.
 * \param [in] interface_actions The array of actions to run.
 * \return A handle on the execution status of the actions, or NULL in case of error.
 */
max_run_t *PacketCaptureClient_enableRemoteCapture_run_array_nonblock(max_engarray_t *engarray, PacketCaptureClient_enableRemoteCapture_actions_t *interface_actions[]);

/**
 * \brief Converts a static-interface action struct into a dynamic-interface max_actions_t struct.
 *
 * Note that this is an internal utility function used by other functions in the static interface.
 *
 * \param [in] maxfile The maxfile to use.
 * \param [in] interface_actions The interface-specific actions to run.
 * \return The dynamic-interface actions to run, or NULL in case of error.
 */
max_actions_t* PacketCaptureClient_enableRemoteCapture_convert(max_file_t *maxfile, PacketCaptureClient_enableRemoteCapture_actions_t *interface_actions);



/*----------------------------------------------------------------------------*/
/*------------------------ Interface readCaptureData -------------------------*/
/*----------------------------------------------------------------------------*/




/**
 * \brief Basic static function for the interface 'readCaptureData'.
 * 
 * \param [in] param_len Interface Parameter "len".
 * \param [out] outstream_toCpu The stream should be of size (param_len * 32) bytes.
 */
void PacketCaptureClient_readCaptureData(
	uint64_t param_len,
	uint64_t *outstream_toCpu);

/**
 * \brief Basic static non-blocking function for the interface 'readCaptureData'.
 * 
 * Schedule to run on an engine and return immediately.
 * The status of the run can be checked either by ::max_wait or ::max_nowait;
 * note that one of these *must* be called, so that associated memory can be released.
 * 
 * 
 * \param [in] param_len Interface Parameter "len".
 * \param [out] outstream_toCpu The stream should be of size (param_len * 32) bytes.
 * \return A handle on the execution status, or NULL in case of error.
 */
max_run_t *PacketCaptureClient_readCaptureData_nonblock(
	uint64_t param_len,
	uint64_t *outstream_toCpu);

/**
 * \brief Advanced static interface, structure for the engine interface 'readCaptureData'
 * 
 */
typedef struct { 
	uint64_t param_len; /**<  [in] Interface Parameter "len". */
	uint64_t *outstream_toCpu; /**<  [out] The stream should be of size (param_len * 32) bytes. */
} PacketCaptureClient_readCaptureData_actions_t;

/**
 * \brief Advanced static function for the interface 'readCaptureData'.
 * 
 * \param [in] engine The engine on which the actions will be executed.
 * \param [in,out] interface_actions Actions to be executed.
 */
void PacketCaptureClient_readCaptureData_run(
	max_engine_t *engine,
	PacketCaptureClient_readCaptureData_actions_t *interface_actions);

/**
 * \brief Advanced static non-blocking function for the interface 'readCaptureData'.
 *
 * Schedule the actions to run on the engine and return immediately.
 * The status of the run can be checked either by ::max_wait or ::max_nowait;
 * note that one of these *must* be called, so that associated memory can be released.
 *
 * 
 * \param [in] engine The engine on which the actions will be executed.
 * \param [in] interface_actions Actions to be executed.
 * \return A handle on the execution status of the actions, or NULL in case of error.
 */
max_run_t *PacketCaptureClient_readCaptureData_run_nonblock(
	max_engine_t *engine,
	PacketCaptureClient_readCaptureData_actions_t *interface_actions);

/**
 * \brief Group run advanced static function for the interface 'readCaptureData'.
 * 
 * \param [in] group Group to use.
 * \param [in,out] interface_actions Actions to run.
 *
 * Run the actions on the first device available in the group.
 */
void PacketCaptureClient_readCaptureData_run_group(max_group_t *group, PacketCaptureClient_readCaptureData_actions_t *interface_actions);

/**
 * \brief Group run advanced static non-blocking function for the interface 'readCaptureData'.
 * 
 *
 * Schedule the actions to run on the first device available in the group and return immediately.
 * The status of the run must be checked with ::max_wait. 
 * Note that use of ::max_nowait is prohibited with non-blocking running on groups:
 * see the ::max_run_group_nonblock documentation for more explanation.
 *
 * \param [in] group Group to use.
 * \param [in] interface_actions Actions to run.
 * \return A handle on the execution status of the actions, or NULL in case of error.
 */
max_run_t *PacketCaptureClient_readCaptureData_run_group_nonblock(max_group_t *group, PacketCaptureClient_readCaptureData_actions_t *interface_actions);

/**
 * \brief Array run advanced static function for the interface 'readCaptureData'.
 * 
 * \param [in] engarray The array of devices to use.
 * \param [in,out] interface_actions The array of actions to run.
 *
 * Run the array of actions on the array of engines.  The length of interface_actions
 * must match the size of engarray.
 */
void PacketCaptureClient_readCaptureData_run_array(max_engarray_t *engarray, PacketCaptureClient_readCaptureData_actions_t *interface_actions[]);

/**
 * \brief Array run advanced static non-blocking function for the interface 'readCaptureData'.
 * 
 *
 * Schedule to run the array of actions on the array of engines, and return immediately.
 * The length of interface_actions must match the size of engarray.
 * The status of the run can be checked either by ::max_wait or ::max_nowait;
 * note that one of these *must* be called, so that associated memory can be released.
 *
 * \param [in] engarray The array of devices to use.
 * \param [in] interface_actions The array of actions to run.
 * \return A handle on the execution status of the actions, or NULL in case of error.
 */
max_run_t *PacketCaptureClient_readCaptureData_run_array_nonblock(max_engarray_t *engarray, PacketCaptureClient_readCaptureData_actions_t *interface_actions[]);

/**
 * \brief Converts a static-interface action struct into a dynamic-interface max_actions_t struct.
 *
 * Note that this is an internal utility function used by other functions in the static interface.
 *
 * \param [in] maxfile The maxfile to use.
 * \param [in] interface_actions The interface-specific actions to run.
 * \return The dynamic-interface actions to run, or NULL in case of error.
 */
max_actions_t* PacketCaptureClient_readCaptureData_convert(max_file_t *maxfile, PacketCaptureClient_readCaptureData_actions_t *interface_actions);



/*----------------------------------------------------------------------------*/
/*---------------------------- Interface default -----------------------------*/
/*----------------------------------------------------------------------------*/




/**
 * \brief Basic static function for the interface 'default'.
 * 
 * \param [in] inscalar_SenderK_socketA0 Input scalar parameter "SenderK.socketA0".
 * \param [in] inscalar_SenderK_socketA1 Input scalar parameter "SenderK.socketA1".
 * \param [in] inscalar_SenderK_socketB0 Input scalar parameter "SenderK.socketB0".
 * \param [in] inscalar_SenderK_socketB1 Input scalar parameter "SenderK.socketB1".
 * \param [in] inscalar_SenderK_socketsALen Input scalar parameter "SenderK.socketsALen".
 * \param [in] inscalar_SenderK_socketsBLen Input scalar parameter "SenderK.socketsBLen".
 * \param [out] outscalar_SlicerFramer_network_mgmt_stream_from_host_tx_framed_eofCount Output scalar parameter "SlicerFramer_network_mgmt_stream_from_host_tx_framed.eofCount".
 * \param [out] outscalar_SlicerFramer_network_mgmt_stream_from_host_tx_framed_sofCount Output scalar parameter "SlicerFramer_network_mgmt_stream_from_host_tx_framed.sofCount".
 * \param [out] outscalar_ddrFifo_internalFifoFull Output scalar parameter "ddrFifo.internalFifoFull".
 * \param [out] outscalar_ddrFifo_memCmdStalling Output scalar parameter "ddrFifo.memCmdStalling".
 * \param [out] outscalar_ddrFifo_memFifoAlmostEmpty Output scalar parameter "ddrFifo.memFifoAlmostEmpty".
 * \param [out] outscalar_ddrFifo_memFifoAlmostFull Output scalar parameter "ddrFifo.memFifoAlmostFull".
 * \param [out] outscalar_ddrFifo_memFifoFillLevel Output scalar parameter "ddrFifo.memFifoFillLevel".
 * \param [out] outscalar_ddrFifo_memReadPointer Output scalar parameter "ddrFifo.memReadPointer".
 * \param [out] outscalar_ddrFifo_memWritePointer Output scalar parameter "ddrFifo.memWritePointer".
 * \param [out] outscalar_ddrFifo_shouldStallDownstream Output scalar parameter "ddrFifo.shouldStallDownstream".
 * \param [out] outscalar_network_mgmt_stream_to_host_rx_framed_FramedStreamPreprocessor_eofCount Output scalar parameter "network_mgmt_stream_to_host_rx_framed_FramedStreamPreprocessor.eofCount".
 * \param [out] outscalar_network_mgmt_stream_to_host_rx_framed_FramedStreamPreprocessor_sofCount Output scalar parameter "network_mgmt_stream_to_host_rx_framed_FramedStreamPreprocessor.sofCount".
 * \param [in] instream_network_mgmt_stream_from_host_tx_framed Stream "network_mgmt_stream_from_host_tx_framed".
 * \param [in] instream_size_network_mgmt_stream_from_host_tx_framed The size of the stream instream_network_mgmt_stream_from_host_tx_framed in bytes.
 * \param [out] outstream_network_mgmt_stream_to_host_rx_framed Stream "network_mgmt_stream_to_host_rx_framed".
 * \param [in] outstream_size_network_mgmt_stream_to_host_rx_framed The size of the stream outstream_network_mgmt_stream_to_host_rx_framed in bytes.
 * \param [out] outstream_toCpu Stream "toCpu".
 * \param [in] outstream_size_toCpu The size of the stream outstream_toCpu in bytes.
 * \param [in] routing_string A string containing comma-separated "from_name -> to_name" routing commands.
 */
void PacketCaptureClient(
	uint64_t inscalar_SenderK_socketA0,
	uint64_t inscalar_SenderK_socketA1,
	uint64_t inscalar_SenderK_socketB0,
	uint64_t inscalar_SenderK_socketB1,
	uint64_t inscalar_SenderK_socketsALen,
	uint64_t inscalar_SenderK_socketsBLen,
	uint64_t *outscalar_SlicerFramer_network_mgmt_stream_from_host_tx_framed_eofCount,
	uint64_t *outscalar_SlicerFramer_network_mgmt_stream_from_host_tx_framed_sofCount,
	uint64_t *outscalar_ddrFifo_internalFifoFull,
	uint64_t *outscalar_ddrFifo_memCmdStalling,
	uint64_t *outscalar_ddrFifo_memFifoAlmostEmpty,
	uint64_t *outscalar_ddrFifo_memFifoAlmostFull,
	uint64_t *outscalar_ddrFifo_memFifoFillLevel,
	uint64_t *outscalar_ddrFifo_memReadPointer,
	uint64_t *outscalar_ddrFifo_memWritePointer,
	uint64_t *outscalar_ddrFifo_shouldStallDownstream,
	uint64_t *outscalar_network_mgmt_stream_to_host_rx_framed_FramedStreamPreprocessor_eofCount,
	uint64_t *outscalar_network_mgmt_stream_to_host_rx_framed_FramedStreamPreprocessor_sofCount,
	const void *instream_network_mgmt_stream_from_host_tx_framed,
	size_t instream_size_network_mgmt_stream_from_host_tx_framed,
	void *outstream_network_mgmt_stream_to_host_rx_framed,
	size_t outstream_size_network_mgmt_stream_to_host_rx_framed,
	void *outstream_toCpu,
	size_t outstream_size_toCpu,
	const char * routing_string);

/**
 * \brief Basic static non-blocking function for the interface 'default'.
 * 
 * Schedule to run on an engine and return immediately.
 * The status of the run can be checked either by ::max_wait or ::max_nowait;
 * note that one of these *must* be called, so that associated memory can be released.
 * 
 * 
 * \param [in] inscalar_SenderK_socketA0 Input scalar parameter "SenderK.socketA0".
 * \param [in] inscalar_SenderK_socketA1 Input scalar parameter "SenderK.socketA1".
 * \param [in] inscalar_SenderK_socketB0 Input scalar parameter "SenderK.socketB0".
 * \param [in] inscalar_SenderK_socketB1 Input scalar parameter "SenderK.socketB1".
 * \param [in] inscalar_SenderK_socketsALen Input scalar parameter "SenderK.socketsALen".
 * \param [in] inscalar_SenderK_socketsBLen Input scalar parameter "SenderK.socketsBLen".
 * \param [out] outscalar_SlicerFramer_network_mgmt_stream_from_host_tx_framed_eofCount Output scalar parameter "SlicerFramer_network_mgmt_stream_from_host_tx_framed.eofCount".
 * \param [out] outscalar_SlicerFramer_network_mgmt_stream_from_host_tx_framed_sofCount Output scalar parameter "SlicerFramer_network_mgmt_stream_from_host_tx_framed.sofCount".
 * \param [out] outscalar_ddrFifo_internalFifoFull Output scalar parameter "ddrFifo.internalFifoFull".
 * \param [out] outscalar_ddrFifo_memCmdStalling Output scalar parameter "ddrFifo.memCmdStalling".
 * \param [out] outscalar_ddrFifo_memFifoAlmostEmpty Output scalar parameter "ddrFifo.memFifoAlmostEmpty".
 * \param [out] outscalar_ddrFifo_memFifoAlmostFull Output scalar parameter "ddrFifo.memFifoAlmostFull".
 * \param [out] outscalar_ddrFifo_memFifoFillLevel Output scalar parameter "ddrFifo.memFifoFillLevel".
 * \param [out] outscalar_ddrFifo_memReadPointer Output scalar parameter "ddrFifo.memReadPointer".
 * \param [out] outscalar_ddrFifo_memWritePointer Output scalar parameter "ddrFifo.memWritePointer".
 * \param [out] outscalar_ddrFifo_shouldStallDownstream Output scalar parameter "ddrFifo.shouldStallDownstream".
 * \param [out] outscalar_network_mgmt_stream_to_host_rx_framed_FramedStreamPreprocessor_eofCount Output scalar parameter "network_mgmt_stream_to_host_rx_framed_FramedStreamPreprocessor.eofCount".
 * \param [out] outscalar_network_mgmt_stream_to_host_rx_framed_FramedStreamPreprocessor_sofCount Output scalar parameter "network_mgmt_stream_to_host_rx_framed_FramedStreamPreprocessor.sofCount".
 * \param [in] instream_network_mgmt_stream_from_host_tx_framed Stream "network_mgmt_stream_from_host_tx_framed".
 * \param [in] instream_size_network_mgmt_stream_from_host_tx_framed The size of the stream instream_network_mgmt_stream_from_host_tx_framed in bytes.
 * \param [out] outstream_network_mgmt_stream_to_host_rx_framed Stream "network_mgmt_stream_to_host_rx_framed".
 * \param [in] outstream_size_network_mgmt_stream_to_host_rx_framed The size of the stream outstream_network_mgmt_stream_to_host_rx_framed in bytes.
 * \param [out] outstream_toCpu Stream "toCpu".
 * \param [in] outstream_size_toCpu The size of the stream outstream_toCpu in bytes.
 * \param [in] routing_string A string containing comma-separated "from_name -> to_name" routing commands.
 * \return A handle on the execution status, or NULL in case of error.
 */
max_run_t *PacketCaptureClient_nonblock(
	uint64_t inscalar_SenderK_socketA0,
	uint64_t inscalar_SenderK_socketA1,
	uint64_t inscalar_SenderK_socketB0,
	uint64_t inscalar_SenderK_socketB1,
	uint64_t inscalar_SenderK_socketsALen,
	uint64_t inscalar_SenderK_socketsBLen,
	uint64_t *outscalar_SlicerFramer_network_mgmt_stream_from_host_tx_framed_eofCount,
	uint64_t *outscalar_SlicerFramer_network_mgmt_stream_from_host_tx_framed_sofCount,
	uint64_t *outscalar_ddrFifo_internalFifoFull,
	uint64_t *outscalar_ddrFifo_memCmdStalling,
	uint64_t *outscalar_ddrFifo_memFifoAlmostEmpty,
	uint64_t *outscalar_ddrFifo_memFifoAlmostFull,
	uint64_t *outscalar_ddrFifo_memFifoFillLevel,
	uint64_t *outscalar_ddrFifo_memReadPointer,
	uint64_t *outscalar_ddrFifo_memWritePointer,
	uint64_t *outscalar_ddrFifo_shouldStallDownstream,
	uint64_t *outscalar_network_mgmt_stream_to_host_rx_framed_FramedStreamPreprocessor_eofCount,
	uint64_t *outscalar_network_mgmt_stream_to_host_rx_framed_FramedStreamPreprocessor_sofCount,
	const void *instream_network_mgmt_stream_from_host_tx_framed,
	size_t instream_size_network_mgmt_stream_from_host_tx_framed,
	void *outstream_network_mgmt_stream_to_host_rx_framed,
	size_t outstream_size_network_mgmt_stream_to_host_rx_framed,
	void *outstream_toCpu,
	size_t outstream_size_toCpu,
	const char * routing_string);

/**
 * \brief Advanced static interface, structure for the engine interface 'default'
 * 
 */
typedef struct { 
	uint64_t inscalar_SenderK_socketA0; /**<  [in] Input scalar parameter "SenderK.socketA0". */
	uint64_t inscalar_SenderK_socketA1; /**<  [in] Input scalar parameter "SenderK.socketA1". */
	uint64_t inscalar_SenderK_socketB0; /**<  [in] Input scalar parameter "SenderK.socketB0". */
	uint64_t inscalar_SenderK_socketB1; /**<  [in] Input scalar parameter "SenderK.socketB1". */
	uint64_t inscalar_SenderK_socketsALen; /**<  [in] Input scalar parameter "SenderK.socketsALen". */
	uint64_t inscalar_SenderK_socketsBLen; /**<  [in] Input scalar parameter "SenderK.socketsBLen". */
	uint64_t *outscalar_SlicerFramer_network_mgmt_stream_from_host_tx_framed_eofCount; /**<  [out] Output scalar parameter "SlicerFramer_network_mgmt_stream_from_host_tx_framed.eofCount". */
	uint64_t *outscalar_SlicerFramer_network_mgmt_stream_from_host_tx_framed_sofCount; /**<  [out] Output scalar parameter "SlicerFramer_network_mgmt_stream_from_host_tx_framed.sofCount". */
	uint64_t *outscalar_ddrFifo_internalFifoFull; /**<  [out] Output scalar parameter "ddrFifo.internalFifoFull". */
	uint64_t *outscalar_ddrFifo_memCmdStalling; /**<  [out] Output scalar parameter "ddrFifo.memCmdStalling". */
	uint64_t *outscalar_ddrFifo_memFifoAlmostEmpty; /**<  [out] Output scalar parameter "ddrFifo.memFifoAlmostEmpty". */
	uint64_t *outscalar_ddrFifo_memFifoAlmostFull; /**<  [out] Output scalar parameter "ddrFifo.memFifoAlmostFull". */
	uint64_t *outscalar_ddrFifo_memFifoFillLevel; /**<  [out] Output scalar parameter "ddrFifo.memFifoFillLevel". */
	uint64_t *outscalar_ddrFifo_memReadPointer; /**<  [out] Output scalar parameter "ddrFifo.memReadPointer". */
	uint64_t *outscalar_ddrFifo_memWritePointer; /**<  [out] Output scalar parameter "ddrFifo.memWritePointer". */
	uint64_t *outscalar_ddrFifo_shouldStallDownstream; /**<  [out] Output scalar parameter "ddrFifo.shouldStallDownstream". */
	uint64_t *outscalar_network_mgmt_stream_to_host_rx_framed_FramedStreamPreprocessor_eofCount; /**<  [out] Output scalar parameter "network_mgmt_stream_to_host_rx_framed_FramedStreamPreprocessor.eofCount". */
	uint64_t *outscalar_network_mgmt_stream_to_host_rx_framed_FramedStreamPreprocessor_sofCount; /**<  [out] Output scalar parameter "network_mgmt_stream_to_host_rx_framed_FramedStreamPreprocessor.sofCount". */
	const void *instream_network_mgmt_stream_from_host_tx_framed; /**<  [in] Stream "network_mgmt_stream_from_host_tx_framed". */
	size_t instream_size_network_mgmt_stream_from_host_tx_framed; /**<  [in] The size of the stream instream_network_mgmt_stream_from_host_tx_framed in bytes. */
	void *outstream_network_mgmt_stream_to_host_rx_framed; /**<  [out] Stream "network_mgmt_stream_to_host_rx_framed". */
	size_t outstream_size_network_mgmt_stream_to_host_rx_framed; /**<  [in] The size of the stream outstream_network_mgmt_stream_to_host_rx_framed in bytes. */
	void *outstream_toCpu; /**<  [out] Stream "toCpu". */
	size_t outstream_size_toCpu; /**<  [in] The size of the stream outstream_toCpu in bytes. */
	const char * routing_string; /**<  [in] A string containing comma-separated "from_name -> to_name" routing commands. */
} PacketCaptureClient_actions_t;

/**
 * \brief Advanced static function for the interface 'default'.
 * 
 * \param [in] engine The engine on which the actions will be executed.
 * \param [in,out] interface_actions Actions to be executed.
 */
void PacketCaptureClient_run(
	max_engine_t *engine,
	PacketCaptureClient_actions_t *interface_actions);

/**
 * \brief Advanced static non-blocking function for the interface 'default'.
 *
 * Schedule the actions to run on the engine and return immediately.
 * The status of the run can be checked either by ::max_wait or ::max_nowait;
 * note that one of these *must* be called, so that associated memory can be released.
 *
 * 
 * \param [in] engine The engine on which the actions will be executed.
 * \param [in] interface_actions Actions to be executed.
 * \return A handle on the execution status of the actions, or NULL in case of error.
 */
max_run_t *PacketCaptureClient_run_nonblock(
	max_engine_t *engine,
	PacketCaptureClient_actions_t *interface_actions);

/**
 * \brief Group run advanced static function for the interface 'default'.
 * 
 * \param [in] group Group to use.
 * \param [in,out] interface_actions Actions to run.
 *
 * Run the actions on the first device available in the group.
 */
void PacketCaptureClient_run_group(max_group_t *group, PacketCaptureClient_actions_t *interface_actions);

/**
 * \brief Group run advanced static non-blocking function for the interface 'default'.
 * 
 *
 * Schedule the actions to run on the first device available in the group and return immediately.
 * The status of the run must be checked with ::max_wait. 
 * Note that use of ::max_nowait is prohibited with non-blocking running on groups:
 * see the ::max_run_group_nonblock documentation for more explanation.
 *
 * \param [in] group Group to use.
 * \param [in] interface_actions Actions to run.
 * \return A handle on the execution status of the actions, or NULL in case of error.
 */
max_run_t *PacketCaptureClient_run_group_nonblock(max_group_t *group, PacketCaptureClient_actions_t *interface_actions);

/**
 * \brief Array run advanced static function for the interface 'default'.
 * 
 * \param [in] engarray The array of devices to use.
 * \param [in,out] interface_actions The array of actions to run.
 *
 * Run the array of actions on the array of engines.  The length of interface_actions
 * must match the size of engarray.
 */
void PacketCaptureClient_run_array(max_engarray_t *engarray, PacketCaptureClient_actions_t *interface_actions[]);

/**
 * \brief Array run advanced static non-blocking function for the interface 'default'.
 * 
 *
 * Schedule to run the array of actions on the array of engines, and return immediately.
 * The length of interface_actions must match the size of engarray.
 * The status of the run can be checked either by ::max_wait or ::max_nowait;
 * note that one of these *must* be called, so that associated memory can be released.
 *
 * \param [in] engarray The array of devices to use.
 * \param [in] interface_actions The array of actions to run.
 * \return A handle on the execution status of the actions, or NULL in case of error.
 */
max_run_t *PacketCaptureClient_run_array_nonblock(max_engarray_t *engarray, PacketCaptureClient_actions_t *interface_actions[]);

/**
 * \brief Converts a static-interface action struct into a dynamic-interface max_actions_t struct.
 *
 * Note that this is an internal utility function used by other functions in the static interface.
 *
 * \param [in] maxfile The maxfile to use.
 * \param [in] interface_actions The interface-specific actions to run.
 * \return The dynamic-interface actions to run, or NULL in case of error.
 */
max_actions_t* PacketCaptureClient_convert(max_file_t *maxfile, PacketCaptureClient_actions_t *interface_actions);

/**
 * \brief Initialise a maxfile.
 */
max_file_t* PacketCaptureClient_init(void);

/* Error handling functions */
int PacketCaptureClient_has_errors(void);
const char* PacketCaptureClient_get_errors(void);
void PacketCaptureClient_clear_errors(void);
/* Free statically allocated maxfile data */
void PacketCaptureClient_free(void);
/* returns: -1 = error running command; 0 = no error reported */
int PacketCaptureClient_simulator_start(void);
/* returns: -1 = error running command; 0 = no error reported */
int PacketCaptureClient_simulator_stop(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* SLIC_DECLARATIONS_PacketCaptureClient_H */

