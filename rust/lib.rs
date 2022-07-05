#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!("bindings.rs");

#[cfg(test)]
mod tests {
    use super::*;
    //use libc::*;
    use std::ffi::{CString, CStr};

    #[test]
    fn testing() {
        unsafe {
            let node_id = "023e60311d349ae482eebec19ad81dd2544b98fdd8151516fcf23d7bf6f55b0ed5";
            let host = "127.0.0.1";
            let rune = "lhE8z3BpHyqffEDDvhumRhl16MtNEU7g1CrZ6i-y2pg9MSZtZXRob2RebGlzdHxtZXRob2ReZ2V0fG1ldGhvZD1zdW1tYXJ5Jm1ldGhvZC9nZXRzaGFyZWRzZWNyZXQmbWV0aG9kL2xpc3RkYXRhc3RvcmU=";
            
            let mut socket: lnsocket = {
                let sock = lnsocket_create();
                ffi_helpers::null_pointer_check!(sock);
                *sock
            };

            lnsocket_genkey(&mut socket);

            let res_connect = {
                let c_node_id = CString::new(node_id).unwrap();
                let c_host = CString::new(host).unwrap();
                lnsocket_connect_tor(
                    &mut socket,
                    c_node_id.as_ptr(),
                    c_host.as_ptr(),
                    std::ptr::null(),
                )
            };
            assert_eq!(res_connect, 1);

            let res_perform_init = lnsocket_perform_init(&mut socket);
            assert_eq!(res_perform_init, 1);

            let method = "getinfo";
            let params = "[]";
            let msg = format!("{{\"method\":\"{}\",\"params\":{},\"rune\":\"{}\"}}", method, params, rune);
            let res_write = {
                let mut cmd = vec![];
                cmd.append(&mut 0x4c4fu16.to_be_bytes().to_vec());
                cmd.append(&mut 1u64.to_be_bytes().to_vec());
                cmd.append(&mut msg.as_bytes().to_vec());
                assert_eq!(cmd.len(), msg.len() + 10);
                lnsocket_write(&mut socket, cmd.as_ptr(), cmd.len() as u16)
            };
            assert_eq!(res_write, 1);

            let response = {
                let mut len = 0u16;
                let mut t: u8 = 0;
                let addr = &mut t as *mut u8 as usize;
                let mut uptr = addr as *mut u8;
                let res_recv = lnsocket_read(&mut socket, &mut uptr, &mut len);
                assert_eq!(res_recv, 1);
                let iptr = uptr as *mut i8;
                CStr::from_ptr(iptr.offset(10)).to_str().unwrap().to_string()
            };
            assert!(response.len() > 0);

            lnsocket_destroy(&mut socket);
        }
    }
}
