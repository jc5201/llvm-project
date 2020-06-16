#include "X86MCInstLower.h"

namespace llvm {
namespace gfree {

X86MCInstLower::X86MCInstLower(const MachineFunction &mf,
                               X86AsmPrinter &asmprinter)
    : Ctx(mf.getContext()), MF(mf), TM(mf.getTarget()), MAI(*TM.getMCAsmInfo()),
      AsmPrinter(asmprinter) {}

MachineModuleInfoMachO &X86MCInstLower::getMachOMMI() const {
  return MF.getMMI().getObjFileInfo<MachineModuleInfoMachO>();
}

/// GetSymbolFromOperand - Lower an MO_GlobalAddress or MO_ExternalSymbol
/// operand to an MCSymbol.
MCSymbol *X86MCInstLower::GetSymbolFromOperand(const MachineOperand &MO) const {
  const DataLayout &DL = MF.getDataLayout();
  assert((MO.isGlobal() || MO.isSymbol() || MO.isMBB()) &&
         "Isn't a symbol reference");

  MCSymbol *Sym = nullptr;
  SmallString<128> Name;
  StringRef Suffix;

  switch (MO.getTargetFlags()) {
  case X86II::MO_DLLIMPORT:
    // Handle dllimport linkage.
    Name += "__imp_";
    break;
  case X86II::MO_COFFSTUB:
    Name += ".refptr.";
    break;
  case X86II::MO_DARWIN_NONLAZY:
  case X86II::MO_DARWIN_NONLAZY_PIC_BASE:
    Suffix = "$non_lazy_ptr";
    break;
  }

  if (!Suffix.empty())
    Name += DL.getPrivateGlobalPrefix();

  if (MO.isGlobal()) {
    const GlobalValue *GV = MO.getGlobal();
    AsmPrinter.getNameWithPrefix(Name, GV);
  } else if (MO.isSymbol()) {
    Mangler::getNameWithPrefix(Name, MO.getSymbolName(), DL);
  } else if (MO.isMBB()) {
    assert(Suffix.empty());
    Sym = MO.getMBB()->getSymbol();
  }

  Name += Suffix;
  if (!Sym)
    Sym = Ctx.getOrCreateSymbol(Name);

  // If the target flags on the operand changes the name of the symbol, do that
  // before we return the symbol.
  switch (MO.getTargetFlags()) {
  default:
    break;
  case X86II::MO_COFFSTUB: {
    MachineModuleInfoCOFF &MMICOFF =
        MF.getMMI().getObjFileInfo<MachineModuleInfoCOFF>();
    MachineModuleInfoImpl::StubValueTy &StubSym = MMICOFF.getGVStubEntry(Sym);
    if (!StubSym.getPointer()) {
      assert(MO.isGlobal() && "Extern symbol not handled yet");
      StubSym = MachineModuleInfoImpl::StubValueTy(
          AsmPrinter.getSymbol(MO.getGlobal()), true);
    }
    break;
  }
  case X86II::MO_DARWIN_NONLAZY:
  case X86II::MO_DARWIN_NONLAZY_PIC_BASE: {
    MachineModuleInfoImpl::StubValueTy &StubSym =
        getMachOMMI().getGVStubEntry(Sym);
    if (!StubSym.getPointer()) {
      assert(MO.isGlobal() && "Extern symbol not handled yet");
      StubSym = MachineModuleInfoImpl::StubValueTy(
          AsmPrinter.getSymbol(MO.getGlobal()),
          !MO.getGlobal()->hasInternalLinkage());
    }
    break;
  }
  }

  return Sym;
}

MCOperand X86MCInstLower::LowerSymbolOperand(const MachineOperand &MO,
                                             MCSymbol *Sym) const {
  // FIXME: We would like an efficient form for this, so we don't have to do a
  // lot of extra uniquing.
  const MCExpr *Expr = nullptr;
  MCSymbolRefExpr::VariantKind RefKind = MCSymbolRefExpr::VK_None;

  switch (MO.getTargetFlags()) {
  default:
    llvm_unreachable("Unknown target flag on GV operand");
  case X86II::MO_NO_FLAG: // No flag.
  // These affect the name of the symbol, not any suffix.
  case X86II::MO_DARWIN_NONLAZY:
  case X86II::MO_DLLIMPORT:
  case X86II::MO_COFFSTUB:
    break;

  case X86II::MO_TLVP:
    RefKind = MCSymbolRefExpr::VK_TLVP;
    break;
  case X86II::MO_TLVP_PIC_BASE:
    Expr = MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_TLVP, Ctx);
    // Subtract the pic base.
    Expr = MCBinaryExpr::createSub(
        Expr, MCSymbolRefExpr::create(MF.getPICBaseSymbol(), Ctx), Ctx);
    break;
  case X86II::MO_SECREL:
    RefKind = MCSymbolRefExpr::VK_SECREL;
    break;
  case X86II::MO_TLSGD:
    RefKind = MCSymbolRefExpr::VK_TLSGD;
    break;
  case X86II::MO_TLSLD:
    RefKind = MCSymbolRefExpr::VK_TLSLD;
    break;
  case X86II::MO_TLSLDM:
    RefKind = MCSymbolRefExpr::VK_TLSLDM;
    break;
  case X86II::MO_GOTTPOFF:
    RefKind = MCSymbolRefExpr::VK_GOTTPOFF;
    break;
  case X86II::MO_INDNTPOFF:
    RefKind = MCSymbolRefExpr::VK_INDNTPOFF;
    break;
  case X86II::MO_TPOFF:
    RefKind = MCSymbolRefExpr::VK_TPOFF;
    break;
  case X86II::MO_DTPOFF:
    RefKind = MCSymbolRefExpr::VK_DTPOFF;
    break;
  case X86II::MO_NTPOFF:
    RefKind = MCSymbolRefExpr::VK_NTPOFF;
    break;
  case X86II::MO_GOTNTPOFF:
    RefKind = MCSymbolRefExpr::VK_GOTNTPOFF;
    break;
  case X86II::MO_GOTPCREL:
    RefKind = MCSymbolRefExpr::VK_GOTPCREL;
    break;
  case X86II::MO_GOT:
    RefKind = MCSymbolRefExpr::VK_GOT;
    break;
  case X86II::MO_GOTOFF:
    RefKind = MCSymbolRefExpr::VK_GOTOFF;
    break;
  case X86II::MO_PLT:
    RefKind = MCSymbolRefExpr::VK_PLT;
    break;
  case X86II::MO_ABS8:
    RefKind = MCSymbolRefExpr::VK_X86_ABS8;
    break;
  case X86II::MO_PIC_BASE_OFFSET:
  case X86II::MO_DARWIN_NONLAZY_PIC_BASE:
    Expr = MCSymbolRefExpr::create(Sym, Ctx);
    // Subtract the pic base.
    Expr = MCBinaryExpr::createSub(
        Expr, MCSymbolRefExpr::create(MF.getPICBaseSymbol(), Ctx), Ctx);
    if (MO.isJTI()) {
      assert(MAI.doesSetDirectiveSuppressReloc());
      // If .set directive is supported, use it to reduce the number of
      // relocations the assembler will generate for differences between
      // local labels. This is only safe when the symbols are in the same
      // section so we are restricting it to jumptable references.
      MCSymbol *Label = Ctx.createTempSymbol();
      AsmPrinter.OutStreamer->EmitAssignment(Label, Expr);
      Expr = MCSymbolRefExpr::create(Label, Ctx);
    }
    break;
  }

  if (!Expr)
    Expr = MCSymbolRefExpr::create(Sym, RefKind, Ctx);

  if (!MO.isJTI() && !MO.isMBB() && MO.getOffset())
    Expr = MCBinaryExpr::createAdd(
        Expr, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);
  return MCOperand::createExpr(Expr);
}

/// Simplify FOO $imm, %{al,ax,eax,rax} to FOO $imm, for instruction with
/// a short fixed-register form.
static void SimplifyShortImmForm(MCInst &Inst, unsigned Opcode) {
  unsigned ImmOp = Inst.getNumOperands() - 1;
  assert(Inst.getOperand(0).isReg() &&
         (Inst.getOperand(ImmOp).isImm() || Inst.getOperand(ImmOp).isExpr()) &&
         ((Inst.getNumOperands() == 3 && Inst.getOperand(1).isReg() &&
           Inst.getOperand(0).getReg() == Inst.getOperand(1).getReg()) ||
          Inst.getNumOperands() == 2) &&
         "Unexpected instruction!");

  // Check whether the destination register can be fixed.
  unsigned Reg = Inst.getOperand(0).getReg();
  if (Reg != X86::AL && Reg != X86::AX && Reg != X86::EAX && Reg != X86::RAX)
    return;

  // If so, rewrite the instruction.
  MCOperand Saved = Inst.getOperand(ImmOp);
  Inst = MCInst();
  Inst.setOpcode(Opcode);
  Inst.addOperand(Saved);
}

/// If a movsx instruction has a shorter encoding for the used register
/// simplify the instruction to use it instead.
static void SimplifyMOVSX(MCInst &Inst) {
  unsigned NewOpcode = 0;
  unsigned Op0 = Inst.getOperand(0).getReg(), Op1 = Inst.getOperand(1).getReg();
  switch (Inst.getOpcode()) {
  default:
    llvm_unreachable("Unexpected instruction!");
  case X86::MOVSX16rr8: // movsbw %al, %ax   --> cbtw
    if (Op0 == X86::AX && Op1 == X86::AL)
      NewOpcode = X86::CBW;
    break;
  case X86::MOVSX32rr16: // movswl %ax, %eax  --> cwtl
    if (Op0 == X86::EAX && Op1 == X86::AX)
      NewOpcode = X86::CWDE;
    break;
  case X86::MOVSX64rr32: // movslq %eax, %rax --> cltq
    if (Op0 == X86::RAX && Op1 == X86::EAX)
      NewOpcode = X86::CDQE;
    break;
  }

  if (NewOpcode != 0) {
    Inst = MCInst();
    Inst.setOpcode(NewOpcode);
  }
}

/// Simplify things like MOV32rm to MOV32o32a.
static void SimplifyShortMoveForm(X86AsmPrinter &Printer, MCInst &Inst,
                                  unsigned Opcode) {
  // Don't make these simplifications in 64-bit mode; other assemblers don't
  // perform them because they make the code larger.
  if (Printer.getSubtarget().is64Bit())
    return;

  bool IsStore = Inst.getOperand(0).isReg() && Inst.getOperand(1).isReg();
  unsigned AddrBase = IsStore;
  unsigned RegOp = IsStore ? 0 : 5;
  unsigned AddrOp = AddrBase + 3;
  assert(
      Inst.getNumOperands() == 6 && Inst.getOperand(RegOp).isReg() &&
      Inst.getOperand(AddrBase + X86::AddrBaseReg).isReg() &&
      Inst.getOperand(AddrBase + X86::AddrScaleAmt).isImm() &&
      Inst.getOperand(AddrBase + X86::AddrIndexReg).isReg() &&
      Inst.getOperand(AddrBase + X86::AddrSegmentReg).isReg() &&
      (Inst.getOperand(AddrOp).isExpr() || Inst.getOperand(AddrOp).isImm()) &&
      "Unexpected instruction!");

  // Check whether the destination register can be fixed.
  unsigned Reg = Inst.getOperand(RegOp).getReg();
  if (Reg != X86::AL && Reg != X86::AX && Reg != X86::EAX && Reg != X86::RAX)
    return;

  // Check whether this is an absolute address.
  // FIXME: We know TLVP symbol refs aren't, but there should be a better way
  // to do this here.
  bool Absolute = true;
  if (Inst.getOperand(AddrOp).isExpr()) {
    const MCExpr *MCE = Inst.getOperand(AddrOp).getExpr();
    if (const MCSymbolRefExpr *SRE = dyn_cast<MCSymbolRefExpr>(MCE))
      if (SRE->getKind() == MCSymbolRefExpr::VK_TLVP)
        Absolute = false;
  }

  if (Absolute &&
      (Inst.getOperand(AddrBase + X86::AddrBaseReg).getReg() != 0 ||
       Inst.getOperand(AddrBase + X86::AddrScaleAmt).getImm() != 1 ||
       Inst.getOperand(AddrBase + X86::AddrIndexReg).getReg() != 0))
    return;

  // If so, rewrite the instruction.
  MCOperand Saved = Inst.getOperand(AddrOp);
  MCOperand Seg = Inst.getOperand(AddrBase + X86::AddrSegmentReg);
  Inst = MCInst();
  Inst.setOpcode(Opcode);
  Inst.addOperand(Saved);
  Inst.addOperand(Seg);
}

static unsigned getRetOpcode(const X86Subtarget &Subtarget) {
  return Subtarget.is64Bit() ? X86::RETQ : X86::RETL;
}

Optional<MCOperand>
X86MCInstLower::LowerMachineOperand(const MachineInstr *MI,
                                    const MachineOperand &MO) const {
  switch (MO.getType()) {
  default:
    MI->print(errs());
    llvm_unreachable("unknown operand type");
  case MachineOperand::MO_Register:
    // Ignore all implicit register operands.
    if (MO.isImplicit())
      return None;
    return MCOperand::createReg(MO.getReg());
  case MachineOperand::MO_Immediate:
    return MCOperand::createImm(MO.getImm());
  case MachineOperand::MO_MachineBasicBlock:
  case MachineOperand::MO_GlobalAddress:
  case MachineOperand::MO_ExternalSymbol:
    return LowerSymbolOperand(MO, GetSymbolFromOperand(MO));
  case MachineOperand::MO_MCSymbol:
    return LowerSymbolOperand(MO, MO.getMCSymbol());
  case MachineOperand::MO_JumpTableIndex:
    return LowerSymbolOperand(MO, AsmPrinter.GetJTISymbol(MO.getIndex()));
  case MachineOperand::MO_ConstantPoolIndex:
  {
    const DataLayout &DL = MF.getMMI().getModule()->getDataLayout();
    MCSymbol *MCS = Ctx.getOrCreateSymbol(Twine(DL.getPrivateGlobalPrefix()) +
                                       "CPI" + Twine(MF.getFunctionNumber()) + "_" +
                                       Twine(MO.getIndex()));
    return LowerSymbolOperand(MO, MCS);
  }
  case MachineOperand::MO_BlockAddress:
    return LowerSymbolOperand(
        MO, AsmPrinter.GetBlockAddressSymbol(MO.getBlockAddress()));
  case MachineOperand::MO_RegisterMask:
    // Ignore call clobbers.
    return None;
  }
}

// Replace TAILJMP opcodes with their equivalent opcodes that have encoding
// information.
static unsigned convertTailJumpOpcode(unsigned Opcode) {
  switch (Opcode) {
  case X86::TAILJMPr:
    Opcode = X86::JMP32r;
    break;
  case X86::TAILJMPm:
    Opcode = X86::JMP32m;
    break;
  case X86::TAILJMPr64:
    Opcode = X86::JMP64r;
    break;
  case X86::TAILJMPm64:
    Opcode = X86::JMP64m;
    break;
  case X86::TAILJMPr64_REX:
    Opcode = X86::JMP64r_REX;
    break;
  case X86::TAILJMPm64_REX:
    Opcode = X86::JMP64m_REX;
    break;
  case X86::TAILJMPd:
  case X86::TAILJMPd64:
    Opcode = X86::JMP_1;
    break;
  case X86::TAILJMPd_CC:
  case X86::TAILJMPd64_CC:
    Opcode = X86::JCC_1;
    break;
  }

  return Opcode;
}

void X86MCInstLower::Lower(const MachineInstr *MI, MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());

  for (const MachineOperand &MO : MI->operands())
    if (auto MaybeMCOp = LowerMachineOperand(MI, MO))
      OutMI.addOperand(MaybeMCOp.getValue());

  // Handle a few special cases to eliminate operand modifiers.
  switch (OutMI.getOpcode()) {
  case X86::LEA64_32r:
  case X86::LEA64r:
  case X86::LEA16r:
  case X86::LEA32r:
    // LEA should have a segment register, but it must be empty.
    assert(OutMI.getNumOperands() == 1 + X86::AddrNumOperands &&
           "Unexpected # of LEA operands");
    assert(OutMI.getOperand(1 + X86::AddrSegmentReg).getReg() == 0 &&
           "LEA has segment specified!");
    break;

  // Commute operands to get a smaller encoding by using VEX.R instead of VEX.B
  // if one of the registers is extended, but other isn't.
  case X86::VMOVZPQILo2PQIrr:
  case X86::VMOVAPDrr:
  case X86::VMOVAPDYrr:
  case X86::VMOVAPSrr:
  case X86::VMOVAPSYrr:
  case X86::VMOVDQArr:
  case X86::VMOVDQAYrr:
  case X86::VMOVDQUrr:
  case X86::VMOVDQUYrr:
  case X86::VMOVUPDrr:
  case X86::VMOVUPDYrr:
  case X86::VMOVUPSrr:
  case X86::VMOVUPSYrr: {
    if (!X86II::isX86_64ExtendedReg(OutMI.getOperand(0).getReg()) &&
        X86II::isX86_64ExtendedReg(OutMI.getOperand(1).getReg())) {
      unsigned NewOpc;
      switch (OutMI.getOpcode()) {
      default: llvm_unreachable("Invalid opcode");
      case X86::VMOVZPQILo2PQIrr: NewOpc = X86::VMOVPQI2QIrr;   break;
      case X86::VMOVAPDrr:        NewOpc = X86::VMOVAPDrr_REV;  break;
      case X86::VMOVAPDYrr:       NewOpc = X86::VMOVAPDYrr_REV; break;
      case X86::VMOVAPSrr:        NewOpc = X86::VMOVAPSrr_REV;  break;
      case X86::VMOVAPSYrr:       NewOpc = X86::VMOVAPSYrr_REV; break;
      case X86::VMOVDQArr:        NewOpc = X86::VMOVDQArr_REV;  break;
      case X86::VMOVDQAYrr:       NewOpc = X86::VMOVDQAYrr_REV; break;
      case X86::VMOVDQUrr:        NewOpc = X86::VMOVDQUrr_REV;  break;
      case X86::VMOVDQUYrr:       NewOpc = X86::VMOVDQUYrr_REV; break;
      case X86::VMOVUPDrr:        NewOpc = X86::VMOVUPDrr_REV;  break;
      case X86::VMOVUPDYrr:       NewOpc = X86::VMOVUPDYrr_REV; break;
      case X86::VMOVUPSrr:        NewOpc = X86::VMOVUPSrr_REV;  break;
      case X86::VMOVUPSYrr:       NewOpc = X86::VMOVUPSYrr_REV; break;
      }
      OutMI.setOpcode(NewOpc);
    }
    break;
  }
  case X86::VMOVSDrr:
  case X86::VMOVSSrr: {
    if (!X86II::isX86_64ExtendedReg(OutMI.getOperand(0).getReg()) &&
        X86II::isX86_64ExtendedReg(OutMI.getOperand(2).getReg())) {
      unsigned NewOpc;
      switch (OutMI.getOpcode()) {
      default: llvm_unreachable("Invalid opcode");
      case X86::VMOVSDrr: NewOpc = X86::VMOVSDrr_REV; break;
      case X86::VMOVSSrr: NewOpc = X86::VMOVSSrr_REV; break;
      }
      OutMI.setOpcode(NewOpc);
    }
    break;
  }

  case X86::VPCMPBZ128rmi:  case X86::VPCMPBZ128rmik:
  case X86::VPCMPBZ128rri:  case X86::VPCMPBZ128rrik:
  case X86::VPCMPBZ256rmi:  case X86::VPCMPBZ256rmik:
  case X86::VPCMPBZ256rri:  case X86::VPCMPBZ256rrik:
  case X86::VPCMPBZrmi:     case X86::VPCMPBZrmik:
  case X86::VPCMPBZrri:     case X86::VPCMPBZrrik:
  case X86::VPCMPDZ128rmi:  case X86::VPCMPDZ128rmik:
  case X86::VPCMPDZ128rmib: case X86::VPCMPDZ128rmibk:
  case X86::VPCMPDZ128rri:  case X86::VPCMPDZ128rrik:
  case X86::VPCMPDZ256rmi:  case X86::VPCMPDZ256rmik:
  case X86::VPCMPDZ256rmib: case X86::VPCMPDZ256rmibk:
  case X86::VPCMPDZ256rri:  case X86::VPCMPDZ256rrik:
  case X86::VPCMPDZrmi:     case X86::VPCMPDZrmik:
  case X86::VPCMPDZrmib:    case X86::VPCMPDZrmibk:
  case X86::VPCMPDZrri:     case X86::VPCMPDZrrik:
  case X86::VPCMPQZ128rmi:  case X86::VPCMPQZ128rmik:
  case X86::VPCMPQZ128rmib: case X86::VPCMPQZ128rmibk:
  case X86::VPCMPQZ128rri:  case X86::VPCMPQZ128rrik:
  case X86::VPCMPQZ256rmi:  case X86::VPCMPQZ256rmik:
  case X86::VPCMPQZ256rmib: case X86::VPCMPQZ256rmibk:
  case X86::VPCMPQZ256rri:  case X86::VPCMPQZ256rrik:
  case X86::VPCMPQZrmi:     case X86::VPCMPQZrmik:
  case X86::VPCMPQZrmib:    case X86::VPCMPQZrmibk:
  case X86::VPCMPQZrri:     case X86::VPCMPQZrrik:
  case X86::VPCMPWZ128rmi:  case X86::VPCMPWZ128rmik:
  case X86::VPCMPWZ128rri:  case X86::VPCMPWZ128rrik:
  case X86::VPCMPWZ256rmi:  case X86::VPCMPWZ256rmik:
  case X86::VPCMPWZ256rri:  case X86::VPCMPWZ256rrik:
  case X86::VPCMPWZrmi:     case X86::VPCMPWZrmik:
  case X86::VPCMPWZrri:     case X86::VPCMPWZrrik: {
    // Turn immediate 0 into the VPCMPEQ instruction.
    if (OutMI.getOperand(OutMI.getNumOperands() - 1).getImm() == 0) {
      unsigned NewOpc;
      switch (OutMI.getOpcode()) {
      default: llvm_unreachable("Invalid opcode");
      case X86::VPCMPBZ128rmi:   NewOpc = X86::VPCMPEQBZ128rm;   break;
      case X86::VPCMPBZ128rmik:  NewOpc = X86::VPCMPEQBZ128rmk;  break;
      case X86::VPCMPBZ128rri:   NewOpc = X86::VPCMPEQBZ128rr;   break;
      case X86::VPCMPBZ128rrik:  NewOpc = X86::VPCMPEQBZ128rrk;  break;
      case X86::VPCMPBZ256rmi:   NewOpc = X86::VPCMPEQBZ256rm;   break;
      case X86::VPCMPBZ256rmik:  NewOpc = X86::VPCMPEQBZ256rmk;  break;
      case X86::VPCMPBZ256rri:   NewOpc = X86::VPCMPEQBZ256rr;   break;
      case X86::VPCMPBZ256rrik:  NewOpc = X86::VPCMPEQBZ256rrk;  break;
      case X86::VPCMPBZrmi:      NewOpc = X86::VPCMPEQBZrm;      break;
      case X86::VPCMPBZrmik:     NewOpc = X86::VPCMPEQBZrmk;     break;
      case X86::VPCMPBZrri:      NewOpc = X86::VPCMPEQBZrr;      break;
      case X86::VPCMPBZrrik:     NewOpc = X86::VPCMPEQBZrrk;     break;
      case X86::VPCMPDZ128rmi:   NewOpc = X86::VPCMPEQDZ128rm;   break;
      case X86::VPCMPDZ128rmib:  NewOpc = X86::VPCMPEQDZ128rmb;  break;
      case X86::VPCMPDZ128rmibk: NewOpc = X86::VPCMPEQDZ128rmbk; break;
      case X86::VPCMPDZ128rmik:  NewOpc = X86::VPCMPEQDZ128rmk;  break;
      case X86::VPCMPDZ128rri:   NewOpc = X86::VPCMPEQDZ128rr;   break;
      case X86::VPCMPDZ128rrik:  NewOpc = X86::VPCMPEQDZ128rrk;  break;
      case X86::VPCMPDZ256rmi:   NewOpc = X86::VPCMPEQDZ256rm;   break;
      case X86::VPCMPDZ256rmib:  NewOpc = X86::VPCMPEQDZ256rmb;  break;
      case X86::VPCMPDZ256rmibk: NewOpc = X86::VPCMPEQDZ256rmbk; break;
      case X86::VPCMPDZ256rmik:  NewOpc = X86::VPCMPEQDZ256rmk;  break;
      case X86::VPCMPDZ256rri:   NewOpc = X86::VPCMPEQDZ256rr;   break;
      case X86::VPCMPDZ256rrik:  NewOpc = X86::VPCMPEQDZ256rrk;  break;
      case X86::VPCMPDZrmi:      NewOpc = X86::VPCMPEQDZrm;      break;
      case X86::VPCMPDZrmib:     NewOpc = X86::VPCMPEQDZrmb;     break;
      case X86::VPCMPDZrmibk:    NewOpc = X86::VPCMPEQDZrmbk;    break;
      case X86::VPCMPDZrmik:     NewOpc = X86::VPCMPEQDZrmk;     break;
      case X86::VPCMPDZrri:      NewOpc = X86::VPCMPEQDZrr;      break;
      case X86::VPCMPDZrrik:     NewOpc = X86::VPCMPEQDZrrk;     break;
      case X86::VPCMPQZ128rmi:   NewOpc = X86::VPCMPEQQZ128rm;   break;
      case X86::VPCMPQZ128rmib:  NewOpc = X86::VPCMPEQQZ128rmb;  break;
      case X86::VPCMPQZ128rmibk: NewOpc = X86::VPCMPEQQZ128rmbk; break;
      case X86::VPCMPQZ128rmik:  NewOpc = X86::VPCMPEQQZ128rmk;  break;
      case X86::VPCMPQZ128rri:   NewOpc = X86::VPCMPEQQZ128rr;   break;
      case X86::VPCMPQZ128rrik:  NewOpc = X86::VPCMPEQQZ128rrk;  break;
      case X86::VPCMPQZ256rmi:   NewOpc = X86::VPCMPEQQZ256rm;   break;
      case X86::VPCMPQZ256rmib:  NewOpc = X86::VPCMPEQQZ256rmb;  break;
      case X86::VPCMPQZ256rmibk: NewOpc = X86::VPCMPEQQZ256rmbk; break;
      case X86::VPCMPQZ256rmik:  NewOpc = X86::VPCMPEQQZ256rmk;  break;
      case X86::VPCMPQZ256rri:   NewOpc = X86::VPCMPEQQZ256rr;   break;
      case X86::VPCMPQZ256rrik:  NewOpc = X86::VPCMPEQQZ256rrk;  break;
      case X86::VPCMPQZrmi:      NewOpc = X86::VPCMPEQQZrm;      break;
      case X86::VPCMPQZrmib:     NewOpc = X86::VPCMPEQQZrmb;     break;
      case X86::VPCMPQZrmibk:    NewOpc = X86::VPCMPEQQZrmbk;    break;
      case X86::VPCMPQZrmik:     NewOpc = X86::VPCMPEQQZrmk;     break;
      case X86::VPCMPQZrri:      NewOpc = X86::VPCMPEQQZrr;      break;
      case X86::VPCMPQZrrik:     NewOpc = X86::VPCMPEQQZrrk;     break;
      case X86::VPCMPWZ128rmi:   NewOpc = X86::VPCMPEQWZ128rm;   break;
      case X86::VPCMPWZ128rmik:  NewOpc = X86::VPCMPEQWZ128rmk;  break;
      case X86::VPCMPWZ128rri:   NewOpc = X86::VPCMPEQWZ128rr;   break;
      case X86::VPCMPWZ128rrik:  NewOpc = X86::VPCMPEQWZ128rrk;  break;
      case X86::VPCMPWZ256rmi:   NewOpc = X86::VPCMPEQWZ256rm;   break;
      case X86::VPCMPWZ256rmik:  NewOpc = X86::VPCMPEQWZ256rmk;  break;
      case X86::VPCMPWZ256rri:   NewOpc = X86::VPCMPEQWZ256rr;   break;
      case X86::VPCMPWZ256rrik:  NewOpc = X86::VPCMPEQWZ256rrk;  break;
      case X86::VPCMPWZrmi:      NewOpc = X86::VPCMPEQWZrm;      break;
      case X86::VPCMPWZrmik:     NewOpc = X86::VPCMPEQWZrmk;     break;
      case X86::VPCMPWZrri:      NewOpc = X86::VPCMPEQWZrr;      break;
      case X86::VPCMPWZrrik:     NewOpc = X86::VPCMPEQWZrrk;     break;
      }

      OutMI.setOpcode(NewOpc);
      OutMI.erase(&OutMI.getOperand(OutMI.getNumOperands() - 1));
      break;
    }

    // Turn immediate 6 into the VPCMPGT instruction.
    if (OutMI.getOperand(OutMI.getNumOperands() - 1).getImm() == 6) {
      unsigned NewOpc;
      switch (OutMI.getOpcode()) {
      default: llvm_unreachable("Invalid opcode");
      case X86::VPCMPBZ128rmi:   NewOpc = X86::VPCMPGTBZ128rm;   break;
      case X86::VPCMPBZ128rmik:  NewOpc = X86::VPCMPGTBZ128rmk;  break;
      case X86::VPCMPBZ128rri:   NewOpc = X86::VPCMPGTBZ128rr;   break;
      case X86::VPCMPBZ128rrik:  NewOpc = X86::VPCMPGTBZ128rrk;  break;
      case X86::VPCMPBZ256rmi:   NewOpc = X86::VPCMPGTBZ256rm;   break;
      case X86::VPCMPBZ256rmik:  NewOpc = X86::VPCMPGTBZ256rmk;  break;
      case X86::VPCMPBZ256rri:   NewOpc = X86::VPCMPGTBZ256rr;   break;
      case X86::VPCMPBZ256rrik:  NewOpc = X86::VPCMPGTBZ256rrk;  break;
      case X86::VPCMPBZrmi:      NewOpc = X86::VPCMPGTBZrm;      break;
      case X86::VPCMPBZrmik:     NewOpc = X86::VPCMPGTBZrmk;     break;
      case X86::VPCMPBZrri:      NewOpc = X86::VPCMPGTBZrr;      break;
      case X86::VPCMPBZrrik:     NewOpc = X86::VPCMPGTBZrrk;     break;
      case X86::VPCMPDZ128rmi:   NewOpc = X86::VPCMPGTDZ128rm;   break;
      case X86::VPCMPDZ128rmib:  NewOpc = X86::VPCMPGTDZ128rmb;  break;
      case X86::VPCMPDZ128rmibk: NewOpc = X86::VPCMPGTDZ128rmbk; break;
      case X86::VPCMPDZ128rmik:  NewOpc = X86::VPCMPGTDZ128rmk;  break;
      case X86::VPCMPDZ128rri:   NewOpc = X86::VPCMPGTDZ128rr;   break;
      case X86::VPCMPDZ128rrik:  NewOpc = X86::VPCMPGTDZ128rrk;  break;
      case X86::VPCMPDZ256rmi:   NewOpc = X86::VPCMPGTDZ256rm;   break;
      case X86::VPCMPDZ256rmib:  NewOpc = X86::VPCMPGTDZ256rmb;  break;
      case X86::VPCMPDZ256rmibk: NewOpc = X86::VPCMPGTDZ256rmbk; break;
      case X86::VPCMPDZ256rmik:  NewOpc = X86::VPCMPGTDZ256rmk;  break;
      case X86::VPCMPDZ256rri:   NewOpc = X86::VPCMPGTDZ256rr;   break;
      case X86::VPCMPDZ256rrik:  NewOpc = X86::VPCMPGTDZ256rrk;  break;
      case X86::VPCMPDZrmi:      NewOpc = X86::VPCMPGTDZrm;      break;
      case X86::VPCMPDZrmib:     NewOpc = X86::VPCMPGTDZrmb;     break;
      case X86::VPCMPDZrmibk:    NewOpc = X86::VPCMPGTDZrmbk;    break;
      case X86::VPCMPDZrmik:     NewOpc = X86::VPCMPGTDZrmk;     break;
      case X86::VPCMPDZrri:      NewOpc = X86::VPCMPGTDZrr;      break;
      case X86::VPCMPDZrrik:     NewOpc = X86::VPCMPGTDZrrk;     break;
      case X86::VPCMPQZ128rmi:   NewOpc = X86::VPCMPGTQZ128rm;   break;
      case X86::VPCMPQZ128rmib:  NewOpc = X86::VPCMPGTQZ128rmb;  break;
      case X86::VPCMPQZ128rmibk: NewOpc = X86::VPCMPGTQZ128rmbk; break;
      case X86::VPCMPQZ128rmik:  NewOpc = X86::VPCMPGTQZ128rmk;  break;
      case X86::VPCMPQZ128rri:   NewOpc = X86::VPCMPGTQZ128rr;   break;
      case X86::VPCMPQZ128rrik:  NewOpc = X86::VPCMPGTQZ128rrk;  break;
      case X86::VPCMPQZ256rmi:   NewOpc = X86::VPCMPGTQZ256rm;   break;
      case X86::VPCMPQZ256rmib:  NewOpc = X86::VPCMPGTQZ256rmb;  break;
      case X86::VPCMPQZ256rmibk: NewOpc = X86::VPCMPGTQZ256rmbk; break;
      case X86::VPCMPQZ256rmik:  NewOpc = X86::VPCMPGTQZ256rmk;  break;
      case X86::VPCMPQZ256rri:   NewOpc = X86::VPCMPGTQZ256rr;   break;
      case X86::VPCMPQZ256rrik:  NewOpc = X86::VPCMPGTQZ256rrk;  break;
      case X86::VPCMPQZrmi:      NewOpc = X86::VPCMPGTQZrm;      break;
      case X86::VPCMPQZrmib:     NewOpc = X86::VPCMPGTQZrmb;     break;
      case X86::VPCMPQZrmibk:    NewOpc = X86::VPCMPGTQZrmbk;    break;
      case X86::VPCMPQZrmik:     NewOpc = X86::VPCMPGTQZrmk;     break;
      case X86::VPCMPQZrri:      NewOpc = X86::VPCMPGTQZrr;      break;
      case X86::VPCMPQZrrik:     NewOpc = X86::VPCMPGTQZrrk;     break;
      case X86::VPCMPWZ128rmi:   NewOpc = X86::VPCMPGTWZ128rm;   break;
      case X86::VPCMPWZ128rmik:  NewOpc = X86::VPCMPGTWZ128rmk;  break;
      case X86::VPCMPWZ128rri:   NewOpc = X86::VPCMPGTWZ128rr;   break;
      case X86::VPCMPWZ128rrik:  NewOpc = X86::VPCMPGTWZ128rrk;  break;
      case X86::VPCMPWZ256rmi:   NewOpc = X86::VPCMPGTWZ256rm;   break;
      case X86::VPCMPWZ256rmik:  NewOpc = X86::VPCMPGTWZ256rmk;  break;
      case X86::VPCMPWZ256rri:   NewOpc = X86::VPCMPGTWZ256rr;   break;
      case X86::VPCMPWZ256rrik:  NewOpc = X86::VPCMPGTWZ256rrk;  break;
      case X86::VPCMPWZrmi:      NewOpc = X86::VPCMPGTWZrm;      break;
      case X86::VPCMPWZrmik:     NewOpc = X86::VPCMPGTWZrmk;     break;
      case X86::VPCMPWZrri:      NewOpc = X86::VPCMPGTWZrr;      break;
      case X86::VPCMPWZrrik:     NewOpc = X86::VPCMPGTWZrrk;     break;
      }

      OutMI.setOpcode(NewOpc);
      OutMI.erase(&OutMI.getOperand(OutMI.getNumOperands() - 1));
      break;
    }

    break;
  }

  // CALL64r, CALL64pcrel32 - These instructions used to have
  // register inputs modeled as normal uses instead of implicit uses.  As such,
  // they we used to truncate off all but the first operand (the callee). This
  // issue seems to have been fixed at some point. This assert verifies that.
  case X86::CALL64r:
  case X86::CALL64pcrel32:
    assert(OutMI.getNumOperands() == 1 && "Unexpected number of operands!");
    break;

  case X86::EH_RETURN:
  case X86::EH_RETURN64: {
    OutMI = MCInst();
    OutMI.setOpcode(getRetOpcode(AsmPrinter.getSubtarget()));
    break;
  }

  case X86::CLEANUPRET: {
    // Replace CLEANUPRET with the appropriate RET.
    OutMI = MCInst();
    OutMI.setOpcode(getRetOpcode(AsmPrinter.getSubtarget()));
    break;
  }

  case X86::CATCHRET: {
    // Replace CATCHRET with the appropriate RET.
    const X86Subtarget &Subtarget = AsmPrinter.getSubtarget();
    unsigned ReturnReg = Subtarget.is64Bit() ? X86::RAX : X86::EAX;
    OutMI = MCInst();
    OutMI.setOpcode(getRetOpcode(Subtarget));
    OutMI.addOperand(MCOperand::createReg(ReturnReg));
    break;
  }

  // TAILJMPd, TAILJMPd64, TailJMPd_cc - Lower to the correct jump
  // instruction.
  case X86::TAILJMPr:
  case X86::TAILJMPr64:
  case X86::TAILJMPr64_REX:
  case X86::TAILJMPd:
  case X86::TAILJMPd64:
    assert(OutMI.getNumOperands() == 1 && "Unexpected number of operands!");
    OutMI.setOpcode(convertTailJumpOpcode(OutMI.getOpcode()));
    break;

  case X86::TAILJMPd_CC:
  case X86::TAILJMPd64_CC:
    assert(OutMI.getNumOperands() == 2 && "Unexpected number of operands!");
    OutMI.setOpcode(convertTailJumpOpcode(OutMI.getOpcode()));
    break;

  case X86::TAILJMPm:
  case X86::TAILJMPm64:
  case X86::TAILJMPm64_REX:
    assert(OutMI.getNumOperands() == X86::AddrNumOperands &&
           "Unexpected number of operands!");
    OutMI.setOpcode(convertTailJumpOpcode(OutMI.getOpcode()));
    break;

  case X86::DEC16r:
  case X86::DEC32r:
  case X86::INC16r:
  case X86::INC32r:
    // If we aren't in 64-bit mode we can use the 1-byte inc/dec instructions.
    if (!AsmPrinter.getSubtarget().is64Bit()) {
      unsigned Opcode;
      switch (OutMI.getOpcode()) {
      default: llvm_unreachable("Invalid opcode");
      case X86::DEC16r: Opcode = X86::DEC16r_alt; break;
      case X86::DEC32r: Opcode = X86::DEC32r_alt; break;
      case X86::INC16r: Opcode = X86::INC16r_alt; break;
      case X86::INC32r: Opcode = X86::INC32r_alt; break;
      }
      OutMI.setOpcode(Opcode);
    }
    break;

  // We don't currently select the correct instruction form for instructions
  // which have a short %eax, etc. form. Handle this by custom lowering, for
  // now.
  //
  // Note, we are currently not handling the following instructions:
  // MOV64ao8, MOV64o8a
  // XCHG16ar, XCHG32ar, XCHG64ar
  case X86::MOV8mr_NOREX:
  case X86::MOV8mr:
  case X86::MOV8rm_NOREX:
  case X86::MOV8rm:
  case X86::MOV16mr:
  case X86::MOV16rm:
  case X86::MOV32mr:
  case X86::MOV32rm: {
    unsigned NewOpc;
    switch (OutMI.getOpcode()) {
    default: llvm_unreachable("Invalid opcode");
    case X86::MOV8mr_NOREX:
    case X86::MOV8mr:  NewOpc = X86::MOV8o32a; break;
    case X86::MOV8rm_NOREX:
    case X86::MOV8rm:  NewOpc = X86::MOV8ao32; break;
    case X86::MOV16mr: NewOpc = X86::MOV16o32a; break;
    case X86::MOV16rm: NewOpc = X86::MOV16ao32; break;
    case X86::MOV32mr: NewOpc = X86::MOV32o32a; break;
    case X86::MOV32rm: NewOpc = X86::MOV32ao32; break;
    }
    SimplifyShortMoveForm(AsmPrinter, OutMI, NewOpc);
    break;
  }

  case X86::ADC8ri: case X86::ADC16ri: case X86::ADC32ri: case X86::ADC64ri32:
  case X86::ADD8ri: case X86::ADD16ri: case X86::ADD32ri: case X86::ADD64ri32:
  case X86::AND8ri: case X86::AND16ri: case X86::AND32ri: case X86::AND64ri32:
  case X86::CMP8ri: case X86::CMP16ri: case X86::CMP32ri: case X86::CMP64ri32:
  case X86::OR8ri:  case X86::OR16ri:  case X86::OR32ri:  case X86::OR64ri32:
  case X86::SBB8ri: case X86::SBB16ri: case X86::SBB32ri: case X86::SBB64ri32:
  case X86::SUB8ri: case X86::SUB16ri: case X86::SUB32ri: case X86::SUB64ri32:
  case X86::TEST8ri:case X86::TEST16ri:case X86::TEST32ri:case X86::TEST64ri32:
  case X86::XOR8ri: case X86::XOR16ri: case X86::XOR32ri: case X86::XOR64ri32: {
    unsigned NewOpc;
    switch (OutMI.getOpcode()) {
    default: llvm_unreachable("Invalid opcode");
    case X86::ADC8ri:     NewOpc = X86::ADC8i8;    break;
    case X86::ADC16ri:    NewOpc = X86::ADC16i16;  break;
    case X86::ADC32ri:    NewOpc = X86::ADC32i32;  break;
    case X86::ADC64ri32:  NewOpc = X86::ADC64i32;  break;
    case X86::ADD8ri:     NewOpc = X86::ADD8i8;    break;
    case X86::ADD16ri:    NewOpc = X86::ADD16i16;  break;
    case X86::ADD32ri:    NewOpc = X86::ADD32i32;  break;
    case X86::ADD64ri32:  NewOpc = X86::ADD64i32;  break;
    case X86::AND8ri:     NewOpc = X86::AND8i8;    break;
    case X86::AND16ri:    NewOpc = X86::AND16i16;  break;
    case X86::AND32ri:    NewOpc = X86::AND32i32;  break;
    case X86::AND64ri32:  NewOpc = X86::AND64i32;  break;
    case X86::CMP8ri:     NewOpc = X86::CMP8i8;    break;
    case X86::CMP16ri:    NewOpc = X86::CMP16i16;  break;
    case X86::CMP32ri:    NewOpc = X86::CMP32i32;  break;
    case X86::CMP64ri32:  NewOpc = X86::CMP64i32;  break;
    case X86::OR8ri:      NewOpc = X86::OR8i8;     break;
    case X86::OR16ri:     NewOpc = X86::OR16i16;   break;
    case X86::OR32ri:     NewOpc = X86::OR32i32;   break;
    case X86::OR64ri32:   NewOpc = X86::OR64i32;   break;
    case X86::SBB8ri:     NewOpc = X86::SBB8i8;    break;
    case X86::SBB16ri:    NewOpc = X86::SBB16i16;  break;
    case X86::SBB32ri:    NewOpc = X86::SBB32i32;  break;
    case X86::SBB64ri32:  NewOpc = X86::SBB64i32;  break;
    case X86::SUB8ri:     NewOpc = X86::SUB8i8;    break;
    case X86::SUB16ri:    NewOpc = X86::SUB16i16;  break;
    case X86::SUB32ri:    NewOpc = X86::SUB32i32;  break;
    case X86::SUB64ri32:  NewOpc = X86::SUB64i32;  break;
    case X86::TEST8ri:    NewOpc = X86::TEST8i8;   break;
    case X86::TEST16ri:   NewOpc = X86::TEST16i16; break;
    case X86::TEST32ri:   NewOpc = X86::TEST32i32; break;
    case X86::TEST64ri32: NewOpc = X86::TEST64i32; break;
    case X86::XOR8ri:     NewOpc = X86::XOR8i8;    break;
    case X86::XOR16ri:    NewOpc = X86::XOR16i16;  break;
    case X86::XOR32ri:    NewOpc = X86::XOR32i32;  break;
    case X86::XOR64ri32:  NewOpc = X86::XOR64i32;  break;
    }
    SimplifyShortImmForm(OutMI, NewOpc);
    break;
  }

  // Try to shrink some forms of movsx.
  case X86::MOVSX16rr8:
  case X86::MOVSX32rr16:
  case X86::MOVSX64rr32:
    SimplifyMOVSX(OutMI);
    break;

  case X86::VCMPPDrri:
  case X86::VCMPPDYrri:
  case X86::VCMPPSrri:
  case X86::VCMPPSYrri:
  case X86::VCMPSDrr:
  case X86::VCMPSSrr: {
    // Swap the operands if it will enable a 2 byte VEX encoding.
    // FIXME: Change the immediate to improve opportunities?
    if (!X86II::isX86_64ExtendedReg(OutMI.getOperand(1).getReg()) &&
        X86II::isX86_64ExtendedReg(OutMI.getOperand(2).getReg())) {
      unsigned Imm = MI->getOperand(3).getImm() & 0x7;
      switch (Imm) {
      default: break;
      case 0x00: // EQUAL
      case 0x03: // UNORDERED
      case 0x04: // NOT EQUAL
      case 0x07: // ORDERED
        std::swap(OutMI.getOperand(1), OutMI.getOperand(2));
        break;
      }
    }
    break;
  }

  case X86::VMOVHLPSrr:
  case X86::VUNPCKHPDrr:
    // These are not truly commutable so hide them from the default case.
    break;

  default: {
    // If the instruction is a commutable arithmetic instruction we might be
    // able to commute the operands to get a 2 byte VEX prefix.
    uint64_t TSFlags = MI->getDesc().TSFlags;
    if (MI->getDesc().isCommutable() &&
        (TSFlags & X86II::EncodingMask) == X86II::VEX &&
        (TSFlags & X86II::OpMapMask) == X86II::TB &&
        (TSFlags & X86II::FormMask) == X86II::MRMSrcReg &&
        !(TSFlags & X86II::VEX_W) && (TSFlags & X86II::VEX_4V) &&
        OutMI.getNumOperands() == 3) {
      if (!X86II::isX86_64ExtendedReg(OutMI.getOperand(1).getReg()) &&
          X86II::isX86_64ExtendedReg(OutMI.getOperand(2).getReg()))
        std::swap(OutMI.getOperand(1), OutMI.getOperand(2));
    }
    break;
  }
  }
}
}
}
