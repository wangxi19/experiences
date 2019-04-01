## OPENSSL

### RSA

```shell
#Generating rsa private key
openssl genrsa -out private_key.pem 2048

#Using rsa private key to generate public key
#PEM and DER are the encoding format, PEM is plaintext format(like this: ---BEGIN--- ---END---), DER is binary format 
#CRT and CER are the certificate format
#Both four contain the public key
openssl rsa -outform PEM -pubout -in private_key.pem -out pub_key.pem

#or using DER (the binary format)
openssl rsa -outform DER -pubout -in private_key.pem -out pub_key.pem

#Using public key to encrypt
echo "hello" | openssl rsautl -encrypt -pubin -inkey ./public_key.pem > msg.enc

#Using private key to decrypt 
openssl rsautl -decrypt -inkey private_key.pem < msg.enc

#***********************PKCS*******************************
#PKCS(Public-Key Cryptography Standards) 
#PKCS#8(See also RFC 5208), Private-Key Information Syntax Specification Version 1.2
#converting private_key.pem to PKCS8 without encryption
openssl pkcs8 -topk8 -in private_key.pem -out pkcs8_private_key.pem -nocrypt -inform PEM

#converting to PKCS1 (traditional) from PKCS8
openssl pkcs8 -in pkcs8_private_key.pem -out private_key.cp.pem -nocrypt -inform PEM

#using PKCS8 to decrypt msg
openssl rsautl -decrypt -inkey pkcs8_private_key.pem < msg.enc

```

