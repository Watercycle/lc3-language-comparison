use std::fmt;
use cpu::Word;

// Negative, Zero, and Positive
pub enum ConditionCode { N, Z, P }

impl ConditionCode {
    pub fn bit(&self) -> i32 {
        match *self {
            ConditionCode::N => 4,  // 100
            ConditionCode::Z => 2,  // 010
            ConditionCode::P => 1,  // 001
        }
    }

    pub fn from_value(val: Word) -> ConditionCode {
        if val < 0 {
            ConditionCode::N
        } else if val == 0 {
            ConditionCode::Z
        } else {
            ConditionCode::P
        }
    }
}

impl fmt::Display for ConditionCode {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.bit())
    }
}