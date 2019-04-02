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
openssl rsa -outform DER -pubout -in private_key.pem -out pub_key.der

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

#***************encryption padding format******************
#using -pkcs to indicate Using PKCS#1 v1.5 padding (default) 
echo "hello" | openssl rsautl -encrypt -pubin -keyform PEM -inkey public_key.pem | openssl rsautl -decrypt -keyform PEM -inkey pkcs8_private_key.pem -pkcs

#some times, may be need to decrypt other data
#-raw may be is needed
#using -raw indicate without padding in here
openssl rsautl -decrypt -keyform PEM -inkey pkcs8_private_key.pem -raw < other.data.enc

#using -raw to encrypt, the data size(bits) must be equal to key length using to generate rsa private key, above is 2048 bits
#so if using -raw to encrypt, data size must be 256 bytes(2048 bits)
#because rsa algorithm need data is aligned

rm -f /tmp/512.bytes; for ((i=0; i<256; i++)); do echo 2 >> /tmp/512.bytes; done; dd if=/tmp/512.bytes count=1 bs=256 | head -c 256 | openssl rsautl -encrypt -pubin -inkey public_key.pem -raw | openssl rsautl -decrypt -inkey pkcs8_private_key.pem -raw; rm -f /tmp/512.bytes;

```

