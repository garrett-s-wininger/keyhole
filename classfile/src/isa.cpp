#include "isa.h"
#include "serialization.h"
#include "sinks.h"

namespace kh::jvm::isa {

auto build_bytecode(const InstructionSequence &instructions)
    -> std::vector<std::byte> {
  auto bytecode = std::vector<std::byte>{};

  // NOTE(garrett): Actual size != instruction count, we'll want a slightly
  // tweaked version that keeps a dynamic calculation
  bytecode.reserve(instructions.size());

  for (const auto instruction : instructions) {
    std::visit(
        [&bytecode](auto &&i) {
          bytecode.push_back(static_cast<std::byte>(i.opcode));
        },
        instruction);
  }

  return bytecode;
}

auto generate_code(const InstructionSequence &instructions)
    -> std::vector<std::byte> {
  auto bytecode = build_bytecode(instructions);

  const attribute::CodeAttribute code{.max_operand_stack_size = 0,
                                      .max_local_variables = 0,
                                      .bytecode = bytecode,
                                      .exception_table = {},
                                      .attributes = {}};

  auto sink = sinks::VectorSink{};
  serialization::serialize(sink, code);

  return std::move(sink).extract();
}

auto generate_code(std::uint16_t max_stack, std::uint16_t max_locals,
                   const InstructionSequence &instructions)
    -> std::vector<std::byte> {
  auto bytecode = build_bytecode(instructions);

  auto code = attribute::CodeAttribute{.max_operand_stack_size = max_stack,
                                       .max_local_variables = max_locals,
                                       .bytecode = bytecode,
                                       .exception_table = {},
                                       .attributes = {}};

  auto sink = sinks::VectorSink{};
  serialization::serialize(sink, code);

  return std::move(sink).extract();
}

} // namespace kh::jvm::isa
