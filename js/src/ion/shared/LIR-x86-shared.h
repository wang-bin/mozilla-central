/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=99:
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jsion_lir_x86_shared_h__
#define jsion_lir_x86_shared_h__

namespace js {
namespace ion {

class LDivI : public LBinaryMath<1>
{
  public:
    LIR_HEADER(DivI);

    LDivI(const LAllocation &lhs, const LAllocation &rhs, const LDefinition &temp) {
        setOperand(0, lhs);
        setOperand(1, rhs);
        setTemp(0, temp);
    }

    const LDefinition *remainder() {
        return getTemp(0);
    }

    MDiv *mir() const {
        return mir_->toDiv();
    }
};

class LModI : public LBinaryMath<1>
{
  public:
    LIR_HEADER(ModI);

    LModI(const LAllocation &lhs, const LAllocation &rhs) {
        setOperand(0, lhs);
        setOperand(1, rhs);
    }

    const LDefinition *remainder() {
        return getDef(0);
    }
};

class LModPowTwoI : public LInstructionHelper<1,1,0>
{
    const int32 shift_;

  public:
    LIR_HEADER(ModPowTwoI);
    int32 shift() {
        return shift_;
    }

    LModPowTwoI(const LAllocation &lhs, int32 shift)
      : shift_(shift)
    {
        setOperand(0, lhs);
    }

    const LDefinition *remainder() {
        return getDef(0);
    }
};

// Takes a tableswitch with an integer to decide
class LTableSwitch : public LInstructionHelper<0, 1, 2>
{
  public:
    LIR_HEADER(TableSwitch);

    LTableSwitch(const LAllocation &in, const LDefinition &inputCopy,
                 const LDefinition &jumpTablePointer, MTableSwitch *ins)
    {
        setOperand(0, in);
        setTemp(0, inputCopy);
        setTemp(1, jumpTablePointer);
        setMir(ins);
    }

    MTableSwitch *mir() const {
        return mir_->toTableSwitch();
    }

    const LAllocation *index() {
        return getOperand(0);
    }
    const LAllocation *tempInt() {
        return getTemp(0)->output();
    }
    const LAllocation *tempPointer() {
        return getTemp(1)->output();
    }
};

// Guard against an object's shape.
class LGuardShape : public LInstructionHelper<0, 1, 0>
{
  public:
    LIR_HEADER(GuardShape);

    LGuardShape(const LAllocation &in) {
        setOperand(0, in);
    }
    const MGuardShape *mir() const {
        return mir_->toGuardShape();
    }
    const LAllocation *input() {
        return getOperand(0);
    }
};

class LRecompileCheck : public LInstructionHelper<0, 0, 0>
{
  public:
    LIR_HEADER(RecompileCheck);
};

class LInterruptCheck : public LInstructionHelper<0, 0, 0>
{
  public:
    LIR_HEADER(InterruptCheck);
};

class LMulI : public LBinaryMath<0, 1>
{
  public:
    LIR_HEADER(MulI);

    LMulI(const LAllocation &lhs, const LAllocation &rhs,
          const LAllocation &lhsCopy)
    {
        setOperand(0, lhs);
        setOperand(1, rhs);
        setOperand(2, lhsCopy);
    }

    MMul *mir() {
        return mir_->toMul();
    }
    const LAllocation *lhsCopy() {
        return this->getOperand(2);
    }
    const LDefinition *output() {
        return this->getDef(0);
    }
};

} // namespace ion
} // namespace js

#endif // jsion_lir_x86_shared_h__

