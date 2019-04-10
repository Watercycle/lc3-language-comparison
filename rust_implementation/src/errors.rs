use std;

#[derive(Fail, Debug)]
pub enum LC3Error {
    #[fail(display = "{}", _0)]
    Io(#[cause] std::io::Error),

    #[fail(display = "please enter filename of program to be ran")]
    MissingProgramFile,
    #[fail(display = "program file missing header")]
    ProgramMissingHeader,
    #[fail(display = "file has an invalid header")]
    BadProgramHeader,
    #[fail(display = "file has an invalid instructions")]
    BadProgramInstructions,
    #[fail(display = "failed operation; given bad arguement")]
    BadArguement,
    #[fail(display = "failed operation; missing arguement")]
    CommandMissingArguement,
    #[fail(display = "{} is not a recognized command", _0)]
    UnrecognizedConsoleCommand(String),
    #[fail(display = "invalid register; the choices are r0 - r7")]
    InvalidRegister,
    #[fail(display = "CPU is not currently running; unable to run instruction")]
    CpuNotRunning,
    #[fail(display = "This implementation does not support the RTI opcode")]
    RTINotSupported,
    #[fail(display = "Tried calling an unused op code")]
    UnusedOpCode,
    #[fail(display = "This implementation does not support trap code {}", _0)]
    UnsupportedTrapCode(i32),
    #[fail(display = "Tried calling an unrecognized op code {}", _0)]
    UnrecognizedOpCode(i32),

    #[fail(display = "Too lazy to figure it out")]
    Unknown
}

impl From<std::io::Error> for LC3Error {
    fn from(err: std::io::Error) -> LC3Error {
        LC3Error::Io(err)
    }
}

pub type Failable<T> = std::result::Result<T, LC3Error>;
