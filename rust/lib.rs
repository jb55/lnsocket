#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!("bindings.rs");

#[cfg(test)]
mod tests {
    use super::*;
    //use libc::*;
    use std::ffi::{CStr, CString};

    #[test]
    fn testing() {
        unsafe {
            let node_id = "03f3c108ccd536b8526841f0a5c58212bb9e6584a1eb493080e7c1cc34f82dad71";
            let host = "monad.endpoint.jb55.com";
            let rune = "uQux-hID66AX5rFUpkt1p9CU_7DsTMyUC4G5yq7-dcw9MTMmbWV0aG9kPWdldGluZm8=";

            let mut socket: lnsocket = {
                let sock = lnsocket_create();
                ffi_helpers::null_pointer_check!(sock);
                *sock
            };

            lnsocket_genkey(&mut socket);

            let res_connect = {
                let c_node_id = CString::new(node_id).unwrap();
                let c_host = CString::new(host).unwrap();
                lnsocket_connect(&mut socket, c_node_id.as_ptr(), c_host.as_ptr())
            };
            assert_eq!(res_connect, 1);

            let res_perform_init = lnsocket_perform_init(&mut socket);
            assert_eq!(res_perform_init, 1);

            let method = "getinfo";
            let params = "[]";
            let msg = format!(
                "{{\"method\":\"{}\",\"params\":{},\"rune\":\"{}\"}}",
                method, params, rune
            );
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
                let mut typ: u16 = 0;
                let addr = &mut t as *mut u8 as usize;
                let mut uptr = addr as *mut u8;
                let res_recv = lnsocket_recv(&mut socket, &mut typ, &mut uptr, &mut len);
                assert_eq!(res_recv, 1);
                let iptr = uptr as *mut i8;
                if typ == 0x594d {
                    CStr::from_ptr(iptr.offset(8)).to_str().unwrap().to_string()
                } else {
                    "other".to_string()
                }
            };
            assert!(response.len() > 0);

            lnsocket_destroy(&mut socket);
        }
    }
}
