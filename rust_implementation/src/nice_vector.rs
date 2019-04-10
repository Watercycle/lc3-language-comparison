use std::ops::Index;
use std::ops::IndexMut;
use num::Num;
use utils::Indexable;

pub struct Vector<T : Num> {
    pub vals: Vec<T>
}

impl<T: Num + Clone> Vector<T> {
    pub fn new(len: i32) -> Vector<T> {
        Vector {
            vals: vec![T::zero(); len as usize]
        }
    }
}

impl<T: Num, R: Indexable> Index<R> for Vector<T> {
    type Output = T;
    fn index(&self, index: R) -> &T {
        &self.vals[index.as_usize()]
    }
}

impl<T: Num, R: Indexable> IndexMut<R> for Vector<T> {
    fn index_mut(&mut self, index: R) -> &mut T {
        &mut self.vals[index.as_usize()]
    }
}