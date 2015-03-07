package com.maxeler.packetCapture;

import com.maxeler.maxcompiler.v2.kernelcompiler.Kernel;
import com.maxeler.maxcompiler.v2.kernelcompiler.KernelParameters;
import com.maxeler.maxcompiler.v2.kernelcompiler.stdlib.core.IO.DelimiterMode;
import com.maxeler.maxcompiler.v2.kernelcompiler.stdlib.core.IO.NonBlockingInput;
import com.maxeler.maxcompiler.v2.kernelcompiler.stdlib.core.IO.NonBlockingMode;
import com.maxeler.maxcompiler.v2.kernelcompiler.types.composite.DFEStruct;
import com.maxeler.packetCapture.types.CpuCaptureDataType;
import com.maxeler.packetCapture.types.Types;

/**
 * Pads capture data to cpu word boundary
 *
 */
public class CpuFormatterKernel extends Kernel
{
	protected CpuFormatterKernel( KernelParameters parameters, Types types )
	{
		super(parameters);
		flush.disabled();

		NonBlockingInput<DFEStruct> captureInput = io.nonBlockingInput("captureData", types.captureDataType, constant.var(true), 1, DelimiterMode.FRAME_LENGTH, 0, NonBlockingMode.NO_TRICKLING);

		DFEStruct data = CpuCaptureDataType.INSTANCE.createFromCaptureData(captureInput.data);

//		Counter counter = control.count.makeCounter(control.count.makeParams(64).withEnable(captureInput.valid));
//
//		DFEStruct data = CpuCaptureDataType.INSTANCE.newPaddedInstance(this);
//		data[CpuCaptureDataType.TIMESTAMP_DOUBT] <== constant.var(true);
//		data[CpuCaptureDataType.TIMESTAMP_VALID] <== constant.var(true);
//		data[CpuCaptureDataType.TIMESTAMP_VALUE] <== counter.getCount();
//
//		data[CpuCaptureDataType.FRAME_SOF] <== constant.var(true);
//		data[CpuCaptureDataType.FRAME_EOF] <== constant.var(true);
//		data[CpuCaptureDataType.FRAME_MOD] <== dfeUInt(3).newInstance(this, 7);
//		data[CpuCaptureDataType.FRAME_DATA] <== counter.getCount().pack();

		captureInput.valid.simWatch("valid");
		data.simWatch("data");

		io.output("cpuCaptureData", data, data.getType(), captureInput.valid);
	}

}
