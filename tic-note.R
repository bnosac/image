##
## CI keys (one-time)
##

library(openssl)
encode_private_key <- function(key) {
  conn <- textConnection(NULL, "w")
  openssl::write_pem(key, conn, password = NULL)
  private_key <- textConnectionValue(conn)
  close(conn)
  private_key <- paste(private_key, collapse = "\n")
  openssl::base64_encode(charToRaw(private_key))
}
key <- openssl::rsa_keygen()
private_key <- encode_private_key(key)
pub_key <- as.list(key)$pubkey

## Print the id_rsa
cat(list(title = "travis-ci_bnosac/image", key = openssl::write_ssh(pub_key))$key, sep = "\n")
cat(private_key, sep = "\n")

##
## KEY: id_rsa
##   - on Github add the public key in the SSH tab
##   - on Travis add id_rsa to the private key on bnosac/image
##   - on Appveyor add id_rsa as private environment variable
## 
## KEY: GITHUB_PAT
##   - on Github create Personal Access Token in Settings > Developer Settings
##   - on Travis add GITHUB_PAT as private environment variable on bnosac/image
##   - on Appveyor add GITHUB_PAT as private environment variable