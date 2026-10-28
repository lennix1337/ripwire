pub struct Hook { cb: fn() }
impl Hook {
    pub fn fire(&self) { (self.cb)() }
}
pub fn spawn(f: fn()) -> Hook { Hook { cb: f } }
pub fn lib_target() {}
pub fn mk() -> Hook { Hook { cb: lib_target } }
