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
 * Adapts capture data to a consistent format read by the CPU.
 */
public class CpuFormatterKernel extends Kernel
{
	protected CpuFormatterKernel( KernelParameters parameters, Types types )
	{
		super(parameters);
		flush.disabled();

		NonBlockingInput<DFEStruct> captureInput = io.nonBlockingInput("captureData", types.captureDataType, constant.var(true), 1, DelimiterMode.FRAME_LENGTH, 0, NonBlockingMode.NO_TRICKLING);

		DFEStruct data = CpuCaptureDataType.INSTANCE.createFromCaptureData(captureInput.data);

		captureInput.valid.simWatch("valid");
		data.simWatch("data");

		io.output("cpuCaptureData", data, data.getType(), captureInput.valid);
	}

}
