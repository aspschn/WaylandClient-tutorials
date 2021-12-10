use std::collections::BTreeMap;

#[repr(C)]
pub struct bl_ptr_btree {
    btree: *mut BTreeMap<u64, u64>,
}

#[no_mangle]
pub extern "C" fn bl_ptr_btree_new() -> *mut bl_ptr_btree {
    let boxed = Box::new(BTreeMap::<u64, u64>::new());
    let ret = Box::new(bl_ptr_btree {
        btree: Box::into_raw(boxed),
    });

    Box::into_raw(ret)
}

#[no_mangle]
pub extern "C" fn bl_ptr_btree_insert(
    btree: *mut bl_ptr_btree,
    wl_surface: u64,
    bl_surface: u64,
) -> bool {
    unsafe {
        let mut boxed = Box::from_raw((*btree).btree);
        let contains = (*boxed).contains_key(&wl_surface);
        if contains {
            Box::into_raw(boxed);

            return false;
        }

        (*boxed).insert(wl_surface, bl_surface);
        Box::into_raw(boxed);

        return true;
    }
}

#[no_mangle]
pub extern "C" fn bl_ptr_btree_contains(btree: *mut bl_ptr_btree, wl_surface: u64) -> bool {
    unsafe {
        let boxed = Box::from_raw((*btree).btree);
        let ret = (*boxed).contains_key(&wl_surface);

        Box::into_raw(boxed);

        ret
    }
}

#[no_mangle]
pub extern "C" fn bl_ptr_btree_get(btree: *mut bl_ptr_btree, wl_surface: u64) -> u64 {
    unsafe {
        let boxed = Box::from_raw((*btree).btree);
        let got = (*boxed).get(&wl_surface);

        match got {
            Some(val) => {
                let ret = *val;
                Box::into_raw(boxed);

                return ret;
            }
            None => {
                Box::into_raw(boxed);

                return 0
            }
        }
    }
}

#[no_mangle]
pub extern "C" fn bl_ptr_btree_remove(btree: *mut bl_ptr_btree, wl_surface: u64) -> bool {
    unsafe {
        let mut boxed = Box::from_raw((*btree).btree);
        let removed = (*boxed).remove(&wl_surface);

        Box::into_raw(boxed);

        match removed {
            Some(_) => {
                return true;
            }
            None => {
                return false;
            }
        }
    }
}

#[no_mangle]
pub extern "C" fn bl_ptr_btree_free(btree: *mut bl_ptr_btree) {
    unsafe {
        let _boxed_btree = Box::from_raw((*btree).btree);
        let _boxed = Box::from_raw(btree);
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn insert() {
        let map = bl_ptr_btree_new();
        bl_ptr_btree_insert(map, 1, 1);
        bl_ptr_btree_insert(map, 2, 2);
        bl_ptr_btree_insert(map, 3, 30);
        assert_eq!(bl_ptr_btree_contains(map, 1), true);
        bl_ptr_btree_remove(map, 3);
        assert_eq!(bl_ptr_btree_contains(map, 3), false);
        bl_ptr_btree_free(map);
    }
}
