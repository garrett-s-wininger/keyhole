#ifndef ISA_H
#define ISA_H

#include <variant>
#include <vector>

namespace kh::jvm::isa {

enum class Opcode : std::underlying_type_t<std::byte> {
  Breakpoint = 0xca,
  Nop = 0x00,
  ImplementationDependent1 = 0xfe,
  ImplementationDependent2 = 0xff,
  Return = 0xb1,
};

struct NoOperandInstruction {
  Opcode opcode;
};

static constexpr auto Breakpoint = NoOperandInstruction{Opcode::Breakpoint};

static constexpr auto ImplementationDependent1 =
    NoOperandInstruction{Opcode::ImplementationDependent1};

static constexpr auto ImplementationDependent2 =
    NoOperandInstruction{Opcode::ImplementationDependent2};

static constexpr auto Nop = NoOperandInstruction{Opcode::Nop};
static constexpr auto Return = NoOperandInstruction{Opcode::Return};

using Instruction = std::variant<NoOperandInstruction>;
using InstructionSequence = std::vector<Instruction>;

struct ExceptionTableEntry {
  std::uint16_t program_counter_start;
  std::uint16_t program_counter_end;
  std::uint16_t handler_program_counter;
  std::uint16_t exception_class_index;
};

auto generate_code(const InstructionSequence &) -> std::vector<std::byte>;

auto generate_code(std::uint16_t, std::uint16_t, const InstructionSequence &)
    -> std::vector<std::byte>;

} // namespace kh::jvm::isa

#endif // ISA_H
