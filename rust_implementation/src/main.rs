#[macro_use] extern crate failure;
extern crate num;
extern crate num_traits;

use cpu::CPU;
use errors::Failable;
use errors::LC3Error;
use program::Program;
//use std::env;
use utils::{read_console_line, parse_num};

mod utils;
mod program;
mod errors;
mod cpu;
mod condition_code;
mod nice_vector;
mod operation;

fn main() {
//    println!("{}", bits(50, 4, 1));
//    println!("{}", bits(50, 5, 1));
//    println!("{}", bits(59, 4, 0));
//
//    println!("{}", bits(10, 5, 1));
//    println!("{}", bits(5, 5, 1));
//    println!("{}", bits(1, 0, 0));
//    println!("{}", bits(0, 0, 0));
//    println!("{}", bits(3, 0, 0));

    println!("LC3 Simulator");

    let mut cpu = CPU::new();
    match run(&mut cpu) {
        Ok(()) => println!("Successful simulation! Exiting program."),
        Err(error) => println!("Failed to run program: {reason}.", reason=error)
    }
}

pub fn run(cpu: &mut CPU) -> Failable<()> {
    let filename = "./programs/wraparound.hex".to_owned(); // env::args().nth(1).ok_or(LC3Error::MissingProgramFile)?;
    let program = Program::from_file(filename)?;
    cpu.load_program(&program);

    println!("Beginning execution; type h for help");

    loop {
        let input = read_console_line()?;
        if should_quit(&input) { break }

        execute_command(cpu, input)?;
    }

    Ok(())
}

fn execute_command(cpu: &mut CPU, input: String) -> Failable<()> {
    // case 1: newline/whitespace => run a single instruction
    if input.is_empty() {
        cpu.run_one_instruction_cycle()?;
        return Ok(())
    }

    // case 2: command is a number => run that many instructions
    if let Ok(num_cycles) = input.parse::<u32>() {
        cpu.run_many_instruction_cycles(num_cycles)?;
        return Ok(())
    }

    // case 3: parse the command arguements and run the command.
    let words : Vec<String> = input.split_whitespace().map(ToOwned::to_owned).collect();
    let cmd = words.first().unwrap();

    match cmd.as_ref() {
        "?" | "h" => {
            print_help()
        },
        "d" => {
            cpu.print_all_info()
        },
        "sr" => {
            let register: i32 = parse_num(&words, 0)?;
            if register > cpu::NUM_REGISTERS { return Err(LC3Error::InvalidRegister) }

            let value = parse_num(&words, 1)?;
            cpu.set_register_value(register, value);
        },
        "sm" => {
            let mem_addr = parse_num(&words, 0)?;
            let value = parse_num(&words, 1)?;
            cpu.set_memory_address_value(mem_addr, value);
        }
        "g" => {
            let addr = parse_num(&words, 0)?;
            cpu.set_pc_address(addr);
        },
        _ => {
            return Err(LC3Error::UnrecognizedConsoleCommand(cmd.to_owned()))
        }
    }

    return Ok(());
}

fn should_quit(console_input: &String) -> bool {
    console_input.contains("q")
}

fn print_help() {
    println!(r#"Simulator commands:
        h or ? to print this message
        q to quit
        d to print (dump) the cpu info
        g [address] to make the PC go to the the new address
        sm [address] [value] to set the value of a memory address
        sr [reg_num] [value] to set the value of a register
        *Press return to execute a single instruction cycle
        *Enter an integer to execute that many instruction cycles
        NOTE: Addresses and values must be in hex (xNNNN)"#)
}