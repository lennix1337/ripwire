use super::eval::{eval_block, eval_block_generic, CallEval};

pub fn closure_eval(block: &str) -> usize {
    let mut ce = CallEval::new(1, eval_block);
    ce.run(block)
}

pub fn closure_eval_generic(block: &str) -> usize {
    let mut ce = CallEval::new(2, eval_block_generic::<u8>);
    ce.run(block)
}
