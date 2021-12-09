use std::collections::BTreeSet;

#[repr(C)]
pub struct bl_ptr_btree {
    btree: Box<BTreeSet<u64>>,
}

#[no_mangle]
pub extern "C" fn bl_ptr_btree_new() -> bl_ptr_btree {
    let btree = Box::new(BTreeSet::<u64>::new());
    let ret: bl_ptr_btree = bl_ptr_btree { btree: btree };

    ret
}

#[no_mangle]
pub extern "C" fn bl_ptr_btree_insert(btree: *mut bl_ptr_btree, val: u64) -> bool {
    unsafe {
        let ret = (*(*btree).btree).insert(val);

        ret
    }
}

#[no_mangle]
pub extern "C" fn bl_ptr_btree_remove(btree: *mut bl_ptr_btree, val: u64) -> bool {
    unsafe {
        let ret = (*(*btree).btree).remove(&val);

        ret
    }
}

#[no_mangle]
pub extern "C" fn bl_ptr_btree_free(btree: *mut bl_ptr_btree) {
    unsafe {
        let ptr = Box::into_raw((*btree).btree);
        Box::from_raw(ptr);
    }
}

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}
