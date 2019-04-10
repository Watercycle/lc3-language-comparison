use std;
use cpu::CPU;
use std::io::Read;
use errors::Failable;
use errors::LC3Error;
use utils::{SignedBitSelection, UnsignedBitSelection};

//////////////////////////////////////////////////////
// LC3 INSTRUCTION TABLE
//////////////////////////////////////////////////////

pub enum Operation { BR, ADD, LD, ST, JSR, AND, LDR, STR, RTI, NOT, LDI, STI, JMP, ERR, LEA, TRAP }

impl Operation {
    pub fn from_code(op_code: u8) -> Failable<Operation> {
        match op_code {
            0b0000 => Ok(Operation::BR),
            0b0001 => Ok(Operation::ADD),
            0b0010 => Ok(Operation::LD),
            0b0011 => Ok(Operation::ST),
            0b0100 => Ok(Operation::JSR),
            0b0101 => Ok(Operation::AND),
            0b0110 => Ok(Operation::LDR),
            0b0111 => Ok(Operation::STR),
            0b1000 => Ok(Operation::RTI),
            0b1001 => Ok(Operation::NOT),
            0b1010 => Ok(Operation::LDI),
            0b1011 => Ok(Operation::STI),
            0b1100 => Ok(Operation::JMP),
            0b1101 => Ok(Operation::ERR),
            0b1110 => Ok(Operation::LEA),
            0b1111 => Ok(Operation::TRAP),
            _ => Err(LC3Error::UnrecognizedOpCode(op_code as i32))
        }
    }

    pub fn execute(&self, cpu: &mut CPU) -> Failable<()> {
        match *self {
            Operation::BR => instr_br(cpu),
            Operation::ADD => instr_add(cpu),
            Operation::LD => instr_ld(cpu),
            Operation::ST => instr_st(cpu),
            Operation::JSR => instr_jsr(cpu),
            Operation::AND => instr_and(cpu),
            Operation::LDR => instr_ldr(cpu),
            Operation::STR => instr_str(cpu),
            Operation::RTI => instr_rti()?,
            Operation::NOT => instr_not(cpu),
            Operation::LDI => instr_ldi(cpu),
            Operation::STI => instr_sti(cpu),
            Operation::JMP => instr_jmp(cpu),
            Operation::ERR => instr_err()?,
            Operation::LEA => instr_lea(cpu),
            Operation::TRAP => instr_trap(cpu)?
        }

        Ok(())
    }
}
/////////////////////////////////////////////////////
// LC3 INSTRUCTION SET
//////////////////////////////////////////////////////

fn instr_br(cpu: &mut CPU) {
    let nzp = cpu.ir.bits(11, 9);
    let pc_offset = cpu.ir.bits_signed(8, 0);

    if nzp & cpu.cc.bit() != 0 {
        cpu.pc += pc_offset
    }
}

fn instr_add(cpu: &mut CPU) {
    let dr = cpu.ir.bits(11, 9);
    let src1 = cpu.ir.bits(8, 6);
    let encoding = cpu.ir.bits(5, 5);

    if encoding == 0 {
        let src2 = cpu.ir.bits(2, 0);
        let result = cpu.reg[src1] + cpu.reg[src2];
        cpu.set_dr(dr, result);
    } else {
        let imm5 = cpu.ir.bits_signed(4, 0);
        let result = cpu.reg[src1] + imm5;
        cpu.set_dr(dr, result);
    }
}

fn instr_ld(cpu: &mut CPU) {
    let dr = cpu.ir.bits(11, 9);
    let pc_offset = cpu.ir.bits_signed(8, 0);

    let result = cpu.mem[cpu.pc + pc_offset];
    cpu.set_dr(dr, result);
}

fn instr_st(cpu: &mut CPU) {
    let src = cpu.ir.bits(11, 9);
    let pc_offset = cpu.ir.bits_signed(8, 0);

    cpu.mem[(cpu.pc + pc_offset)] = cpu.reg[src];
}

fn instr_jsr(cpu: &mut CPU) {
    let mode = cpu.ir.bits(11, 11);

    cpu.reg[7] = cpu.pc;

    if mode == 1 {
        let pc_offset = cpu.ir.bits_signed(10, 0);
        cpu.pc = cpu.pc + pc_offset
    } else { // JSSR
        let reg_num = cpu.ir.bits(8, 6);
        cpu.pc = cpu.reg[reg_num];
    }
}

fn instr_and(cpu: &mut CPU) {
    let dr = cpu.ir.bits(11, 9);
    let src1 = cpu.ir.bits(8, 6);
    let encoding = cpu.ir.bits(5, 5);

    if encoding == 0 {
        let src2 = cpu.ir.bits(2, 0);

        let result = cpu.reg[src1] & cpu.reg[src2];
        cpu.set_dr(dr, result);
    } else {
        let imm5 = cpu.ir.bits_signed(4,0);

        let result = cpu.reg[src1] & imm5;
        cpu.set_dr(dr, result);
    }
}

fn instr_ldr(cpu: &mut CPU) {
    let dr = cpu.ir.bits(11, 9);
    let base = cpu.ir.bits(8, 6);
    let offset = cpu.ir.bits_signed(5, 0);

    let result = cpu.mem[cpu.reg[base] + offset];
    cpu.set_dr(dr, result);
}

fn instr_str(cpu: &mut CPU) {
    let dr = cpu.ir.bits(11, 9);
    let base = cpu.ir.bits(8, 6);
    let offset = cpu.ir.bits(5, 0);

    cpu.mem[base + offset] = cpu.reg[dr];
}

fn instr_rti() -> Failable<()> {
    Err(LC3Error::RTINotSupported)
}

fn instr_not(cpu: &mut CPU) {
    let dr = cpu.ir.bits(11, 9);
    let src = cpu.ir.bits(8, 6);

    let result = !cpu.reg[src];
    cpu.set_dr(dr, result);
}

fn instr_ldi(cpu: &mut CPU) {
    let dr = cpu.ir.bits(11, 9);
    let pc_offset = cpu.ir.bits_signed(8, 0);

    let result = cpu.mem[cpu.mem[cpu.pc + pc_offset]];
    cpu.set_dr(dr, result);
}

fn instr_sti(cpu: &mut CPU) {
    let src = cpu.ir.bits(11, 9);
    let pc_offset = cpu.ir.bits_signed(8, 0);

    let dest = cpu.mem[cpu.pc + pc_offset];
    cpu.mem[dest] = cpu.reg[src];
}

fn instr_jmp(cpu: &mut CPU) {
    let base = cpu.ir.bits(8, 6);
    cpu.pc = cpu.reg[base];
}

fn instr_err() -> Failable<()> {
    Err(LC3Error::UnusedOpCode)
}

fn instr_lea(cpu: &mut CPU) {
    let dr = cpu.ir.bits(11, 9);
    let pc_offset = cpu.ir.bits_signed(8, 0);

    let result = cpu.pc + pc_offset;
    cpu.set_dr(dr, result);
}

fn instr_trap(cpu: &mut CPU) -> Failable<()> {
    let trap_code = cpu.ir.bits(7, 0);

    match trap_code {
        0x20 => trap_getchar(cpu),
        0x21 => trap_out(cpu),
        0x22 => trap_puts(cpu),
        0x23 => trap_input(cpu),
        0x25 => trap_halt(cpu),
        _ => return Err(LC3Error::UnsupportedTrapCode(trap_code))
    };

    Ok(())
}


//////////////////////////////////////////////////////
// TRAP CODES
//////////////////////////////////////////////////////


fn trap_getchar(cpu: &mut CPU) {
    let input_char = std::io::stdin().bytes().next().ok_or(LC3Error::Unknown).unwrap().unwrap();
    cpu.reg[0] = input_char as i32;
}

fn trap_out(cpu: &mut CPU) {
    print!("{}", (cpu.reg[0] as u8) as char);
}

fn trap_puts(cpu: &mut CPU) {
    let mut temp_pc = cpu.reg[0] as u16;

    while cpu.mem[temp_pc] != 0 {
        print!("{}", cpu.mem[temp_pc]);
        temp_pc = temp_pc.wrapping_add(1);
    }
}

fn trap_input(cpu: &mut CPU) {
    print!("{}", "Enter a character: ");
    trap_getchar(cpu)
}

fn trap_halt(cpu: &mut CPU) {
    println!("{}", "Trap halt reached, halting CPU");
    cpu.running = false;
}