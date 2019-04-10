use std;
use std::fs::File;
use std::io::Read;
use std::path::Path;
use std::io;
use std::io::BufRead;
use errors::LC3Error;
use std::str::FromStr;
use errors::Failable;
use num_traits::PrimInt;

//pub trait UsefulInteger: Integer + BitAnd<isize, Output = isize> + Not<Output = isize> + Add {}
//impl<T> UsefulInteger for T where T: Integer + BitAnd<isize, Output = isize> + Not<Output = isize> + Add {}

pub fn lines_from_file(filename: &AsRef<Path>) -> Result<Vec<String>, std::io::Error> {
    let mut file = File::open(filename)?;

    let mut contents = String::new();
    file.read_to_string(&mut contents)?;

    Ok(contents.lines().map(ToOwned::to_owned).collect())
}

pub fn read_console_line() -> Result<String, std::io::Error> {
    let mut line = String::new();
    let stdin = io::stdin();
    stdin.lock().read_line(&mut line)?;
    line.trim();

    Ok(line)
}

// Unsigned Bit Selection
// bits(59, 4, 0) = 001.11011. = 27
// bits(50, 5, 1) = 00.11001.0 = 25
// bits(10, 5, 1) = 00.00101.0 = 5
// bits(5, 5, 1)  = 00.00010.1 = 2
// bits(1, 0, 0)  = 0000000.1. = 1
// bits(0, 0, 0)  = 0000000.0. = 0


// Signed Bit Selection
// bits_s(50, 4, 1) = 001.1001.0 = -7
// bits_s(50, 5, 1) = 00.11001.0 = -7
// bits_s(59, 4, 0) = 001.11011. = -5
// bits_s(10, 5, 1) = 00.00101.0 = 5
// bits_s(5, 5, 1)  = 00.00010.1 = 2
// bits_s(1, 0, 0)  = 0000000.1. = 1
// bits_s(0, 0, 0)  = 0000000.0. = 0
// bits_s(3, 0, 0)  = 0000001.1. = 1


pub fn parse_num<T: FromStr + PrimInt>(words: &Vec<String>, index: i32) -> Failable<T> {
    let arg = words.get(index as usize).ok_or(LC3Error::CommandMissingArguement)?;
    let num =  arg.parse::<T>() // try to parse decimal
        .or(T::from_str_radix(arg, 16)) // try to parse hex
        .or(Err(LC3Error::BadArguement))?;

    Ok(num)
}

/*
macro_rules! log {
    ($fmt:expr, $($arg:tt)*) => {
        unsafe {
            if LOGGING {
                print!($fmt, $($arg)*);
            }
        }
    }
}
static mut LOGGING: bool = false;
//    log!("{:04X}: {:04X} | ", self.pc, self.ir);

*/

/*
fn bits<T: PrimInt + BitAnd<usize, Output = isize>>(value: T, left: usize, right: usize) -> usize {
    assert!(left >= right, "left must be >= right");

    let number_of_ones: usize = (left - right) + 1;
    let mask: usize = ((1 << number_of_ones) - 1) << right;
    let actual_value: isize = (value & mask) >> right;

    return actual_value as usize;
}


fn bits_s<T: PrimInt + BitAnd<usize, Output = isize>>(value: T, left: usize, right: usize) -> isize {
    assert!(left >= right, "left must be >= right");

    let selection_is_positive = if left == right {
        true
    } else {
        // selection is positive if leftmost bit is 0
        value & (1 << ((left - right) + 1)) == 0
    };

    if selection_is_positive {
        return bits(value, left, right) as isize;
    } else {
        return -((bits(!value, left - 1, right) + 1) as isize);
    }
}

pub trait BitSelection {
    fn bits(self, left: usize, right: usize) -> usize;
    fn bits_s(self, left: usize, right: usize) -> isize;
}

macro_rules! bit_select_impl {
    ($($T:ty)*) => ($(
        impl BitSelection for $T {
            fn bits(self, left: usize, right: usize) -> usize {
                bits(self, left, right)
            }

            fn bits_s(self, left: usize, right: usize) -> isize {
                bits_s(self, left, right)
            }
        }
    )*)
}

    // println!("Set pc to address {:04X}", addr);
    // println!("Set memory address {:04X} to {}.", addr, val);
    // println!("Set r{} value to {}", reg_num, val);

bit_select_impl! { usize u8 u16 u32 u64 u128 isize i8 i16 i32 i64 i128 }
*/

pub trait UnsignedBitSelection {
    fn bits(self, left: i32, right: i32) -> i32;
}


pub trait SignedBitSelection {
    fn bits_signed(self, left: i32, right: i32) -> i32;
}

macro_rules! unsigned_bit_select_impl {
    ($($T:ty)*) => ($(
        impl UnsignedBitSelection for $T {
            fn bits(self, left: i32, right: i32) -> i32 {
                assert!(left >= right, "left must be >= right");

                let number_of_ones = (left - right) + 1;
                let mask = ((1 << number_of_ones) - 1) << right;
                let actual_value = (self & mask) >> right;

                return actual_value as i32;
            }
        }
    )*)
}

macro_rules! signed_bit_select_impl {
    ($($T:ty)*) => ($(
        impl SignedBitSelection for $T {
            fn bits_signed(self, left: i32, right: i32) -> i32 {
                assert!(left >= right, "left must be >= right");

                let selection_is_positive = if left == right {
                    true
                } else {
                    // selection is positive if leftmost bit is 0
                    self & (1 << ((left - right) + 1)) == 0
                };

                if selection_is_positive {
                    return self.bits(left, right) as i32;
                } else {
                    return -(((!self).bits(left - 1, right) + 1) as i32);
                }
            }
        }
    )*)
}


unsigned_bit_select_impl! { usize u8 u16 u32 u64 u128 isize i8 i16 i32 i64 i128 }
signed_bit_select_impl! { usize u8 u16 u32 u64 u128 isize i8 i16 i32 i64 i128 }

pub trait Indexable {
    fn as_usize(self) -> usize;
}

macro_rules! indexable_impl {
    ($($T:ty)*) => ($(
        impl Indexable for $T {
            fn as_usize(self) -> usize {
                self as usize
            }
        }
    )*)
}

indexable_impl! { usize u8 u16 u32 u64 u128 isize i8 i16 i32 i64 i128 }
