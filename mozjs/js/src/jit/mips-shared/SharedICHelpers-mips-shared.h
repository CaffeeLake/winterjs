/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: set ts=8 sts=2 et sw=2 tw=80:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jit_mips_shared_SharedICHelpers_mips_shared_h
#define jit_mips_shared_SharedICHelpers_mips_shared_h

#include "jit/BaselineIC.h"
#include "jit/JitFrames.h"
#include "jit/MacroAssembler.h"
#include "jit/SharedICRegisters.h"

namespace js {
namespace jit {

// Distance from sp to the top Value inside an IC stub (no return address on
// the stack on MIPS).
static const size_t ICStackValueOffset = 0;

struct BaselineStubFrame {
  uintptr_t savedFrame;
  uintptr_t savedStub;
  uintptr_t returnAddress;
  uintptr_t descriptor;
};

// Size of values pushed by EmitBaselineEnterStubFrame.
static const uint32_t STUB_FRAME_SIZE = sizeof(BaselineStubFrame);
static const uint32_t STUB_FRAME_SAVED_STUB_OFFSET =
    offsetof(BaselineStubFrame, savedStub);

inline void EmitRestoreTailCallReg(MacroAssembler& masm) {
  // No-op on MIPS because ra register is always holding the return address.
}

inline void EmitRepushTailCallReg(MacroAssembler& masm) {
  // No-op on MIPS because ra register is always holding the return address.
}

inline void EmitCallIC(MacroAssembler& masm, CodeOffset* callOffset) {
  // The stub pointer must already be in ICStubReg.
  // Load stubcode pointer from the ICStub.
  // R2 won't be active when we call ICs, so we can use it as scratch.
  masm.loadPtr(Address(ICStubReg, ICStub::offsetOfStubCode()), R2.scratchReg());

  // Call the stubcode via a direct jump-and-link
  masm.call(R2.scratchReg());
  *callOffset = CodeOffset(masm.currentOffset());
}

inline void EmitReturnFromIC(MacroAssembler& masm) { masm.branch(ra); }

inline void EmitBaselineLeaveStubFrame(MacroAssembler& masm,
                                       bool calledIntoIon = false) {
  // Ion frames do not save and restore the frame pointer. If we called
  // into Ion, we have to restore the stack pointer from the frame descriptor.
  // If we performed a VM call, the descriptor has been popped already so
  // in that case we use the frame pointer.
  if (calledIntoIon) {
    masm.pop(ScratchRegister);
    masm.rshiftPtr(Imm32(FRAMESIZE_SHIFT), ScratchRegister);
    masm.addPtr(ScratchRegister, BaselineStackReg);
  } else {
    masm.movePtr(BaselineFrameReg, BaselineStackReg);
  }

  masm.loadPtr(Address(StackPointer, offsetof(BaselineStubFrame, savedFrame)),
               BaselineFrameReg);
  masm.loadPtr(Address(StackPointer, offsetof(BaselineStubFrame, savedStub)),
               ICStubReg);

  // Load the return address.
  masm.loadPtr(
      Address(StackPointer, offsetof(BaselineStubFrame, returnAddress)),
      ICTailCallReg);

  // Discard the frame descriptor.
  masm.loadPtr(Address(StackPointer, offsetof(BaselineStubFrame, descriptor)),
               ScratchRegister);
  masm.addPtr(Imm32(STUB_FRAME_SIZE), StackPointer);
}

template <typename AddrType>
inline void EmitPreBarrier(MacroAssembler& masm, const AddrType& addr,
                           MIRType type) {
  // On MIPS, $ra is clobbered by guardedCallPreBarrier. Save it first.
  masm.push(ra);
  masm.guardedCallPreBarrier(addr, type);
  masm.pop(ra);
}

inline void EmitStubGuardFailure(MacroAssembler& masm) {
  // Load next stub into ICStubReg
  masm.loadPtr(Address(ICStubReg, ICCacheIRStub::offsetOfNext()), ICStubReg);

  // Return address is already loaded, just jump to the next stubcode.
  MOZ_ASSERT(ICTailCallReg == ra);
  masm.jump(Address(ICStubReg, ICStub::offsetOfStubCode()));
}

}  // namespace jit
}  // namespace js

#endif /* jit_mips_shared_SharedICHelpers_mips_shared_h */
