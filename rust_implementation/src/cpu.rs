use errors::LC3Error;
use program::Program;
use errors::Failable;
use condition_code::ConditionCode;
use utils::{UnsignedBitSelection};
use nice_vector::Vector;
use operation::Operation;

pub type Word = i32; // actually i16
pub type Address = i32; // actually u16
pub type Instruction = i32; // actually u16

pub const NUM_REGISTERS: i32 = 8;
pub const NUM_MEMORY_ADDRESSES: i32 = 65536;

pub struct CPU {
    pub mem: Vector<Address>, // Memory
    pub reg: Vector<Word>, // Registers
    pub running: bool, // Are instructions being executed?

    pub pc: Address, // Program Counter
    pub ir: Instruction, // Instruction Register
    pub cc: ConditionCode // Condition Code (used for conditional branching)
}

impl CPU {
    pub fn new() -> CPU {
        CPU {
            mem: Vector::new(NUM_MEMORY_ADDRESSES),
            reg: Vector::new(NUM_REGISTERS),
            running: true,
            pc: 0,
            ir: 0,
            cc: ConditionCode::Z
        }
    }

    pub fn load_program(&mut self, program: &Program) -> () {
        self.pc = program.program_counter_start();

        // load program instructions into memory
        let mut temp_pc = self.pc as u16;
        for instr in program.instructions() {
            self.mem[temp_pc] = *instr;
            temp_pc = temp_pc.wrapping_add(1);
        }
    }

    pub fn run_one_instruction_cycle(&mut self) -> Failable<()> {
        if !self.running { return Err(LC3Error::CpuNotRunning) }

        // load current instruction into instruction register
        self.ir = self.mem[self.pc];

        self.pc += 1;
        if self.pc > 0xffff {
            self.pc = 0x0000
        }

        // execute current instruction
        let op_code = self.ir.bits(15, 12) as u8;
        let operation = Operation::from_code(op_code)?;
        self.run_operation(operation)?;

        Ok(())
    }

    pub fn run_operation(&mut self, operation: Operation) -> Failable<()> {
        operation.execute(self)?;
        Ok(())
    }

    // updates condition code based on value being assigned to the given
    // destination register
    pub fn set_dr(&mut self, reg_num: i32, val: Word) {
        self.reg[reg_num] = val;
        self.cc = ConditionCode::from_value(val);
    }

    //////////////////////////////////////////////////////
    // CPU VITALS
    //////////////////////////////////////////////////////

    pub fn print_control_unit(&self) {
        println!("Control Unit:");
        println!("PC = {:04X}    IR = {:04X}    CC = {}    RUNNING: {}",
                 self.pc, self.ir, self.cc, self.running);
    }

    // Prints all (useful) instructions currently in memory in both hex and decimal.
    pub fn print_memory(&self) {
        println!("Memory (addresses x0000 - xFFFF, excluding NOP instructions)");
        for (i, instr) in self.mem.vals.iter().enumerate().filter(|(_i, &x)| x != 0) {
            println!("{:04X}: {:04X}    {}", i, instr, instr)
        }
    }

    // Prints cpu register values in two separate rows.
    pub fn print_register_contents(&self) {
        for i in 0..NUM_REGISTERS {
            if i % (NUM_REGISTERS / 2) == 0 { println!(); }
            println!("R{}: {:04X}  {}	", i, self.reg[i], self.reg[i]);
        }
    }

    pub fn print_all_info(&self) {
        self.print_control_unit();
        self.print_memory();
    }


    //////////////////////////////////////////////////////
    // CPU API
    //////////////////////////////////////////////////////


    pub fn set_pc_address(&mut self, addr: Address) {
        self.pc = addr;
        self.running = true;
    }

    pub fn set_memory_address_value(&mut self, addr: Address, val: Word) {
        self.mem[addr] = val;
    }

    pub fn set_register_value(&mut self, reg_num: i32, val: Word) {
        self.reg[reg_num] = val;
    }


    //////////////////////////////////////////////////////
    // CONVENIENCE METHODS
    //////////////////////////////////////////////////////


    pub fn run_many_instruction_cycles(&mut self, num_cycles: u32) -> Failable<()> {
        for _ in 0..num_cycles {
            self.run_one_instruction_cycle()?;
            if !self.running { break; }
        }

        Ok(())
    }
}