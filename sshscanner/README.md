# Note
for learning only
# Usage
```shell
make dbg=1
while IFS= read -r line; do ./sshscanner $line 2000 1>result.txt 2>error.txt; done < alicloud.txt

```
