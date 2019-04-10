use std;
use errors::LC3Error;
use cpu::{Address};
use errors::Failable;
use utils::lines_from_file;
use cpu::Instruction;

pub struct Program {
    header: Address,
    instructions: Vec<Instruction>
}

impl Program {
    pub fn from_file(filename: String) -> Failable<Program> {
        let liness = lines_from_file(&filename)?;
        let mut lines = liness.iter()
            .map(|line| line.split_whitespace().next().unwrap());

        // .ok_or(Err(LC3Error::ProgramMissingHeader))??
        let first_line = lines.next().unwrap();
        let head = Instruction::from_str_radix(&first_line, 16).or(Err(LC3Error::BadProgramHeader))?;

        let body = std::panic::catch_unwind(|| {
            lines.map(|line| Instruction::from_str_radix(line, 16).unwrap()).collect()
        }).or(Err(LC3Error::BadProgramInstructions))?;

        Ok(Program {
            header: head,
            instructions: body
        })
    }

    pub fn program_counter_start(&self) -> Address {
        self.header
    }

    pub fn instructions(&self) -> &Vec<Instruction> {
        &self.instructions
    }
}