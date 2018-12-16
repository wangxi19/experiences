# Installing vim extension for support golang

- install **vundle.vim**

`$mkdir ~/.vim/bundle`

`$git clone https://github.com/gmarik/Vundle.vim.git ~/.vim/bundle/Vundle.vim`

- install **gmarik/Vundle.vim** && **Valloric/YouCompleteMe** && **dgryski/vim-godef**

`$vim ~/.vimrc`, and add the following content to top of file
```
set nocompatible
set number
filetype off

set rtp+=~/.vim/bundle/Vundle.vim
call vundle#begin()

"Plugin 'VundleVim/Vundle.vim'
Plugin 'gmarik/Vundle.vim'
Plugin 'Valloric/YouCompleteMe'
Plugin 'fatih/vim-go'
Plugin 'dgryski/vim-godef'

call vundle#end()
filetype plugin indent on
syntax on
```
Type `:wq` to save and quit startup script(~/.vimrc), and type `$vim` to reopen vim and loading the new startup script(`~/.vimrc`), then type `:PluginInstall` to install these plugins, ordered type `:w` and `:wq` when you can see **Done** on the bottom of installation page, you on the cmd now, and then type `$cd ~/.vim/bundle/YouCompleteMe/ && ./install.py --gocode-completer` to install the YCM's golang extension

- install **gocode**

`$go get -u github.com/nsf/gocode`

Configure gocode

 `$ cd $GOPATH/src/github.com/nsf/gocode/vim`
 
 `$ ./update.sh`
 
 `$ gocode set propose-builtins true`

 propose-builtins true
 
 `$ gocode set lib-path "/home/border/gocode/pkg/linux_amd64"`
 
 lib-path "/home/border/gocode/pkg/linux_amd64"
 
 `$ gocode set`
 
 propose-builtins true
 
 lib-path "/home/border/gocode/pkg/linux_amd64"
 
Explanation of gocode configuration:
propose-builtins: specifies whether or not to open intelligent completion; false by default. lib-path: gocode only searches for packages in $GOPATH/pkg/$GOOS_$GOARCH and $GOROOT/pkg/$GOOS_$GOARCH. This setting can be used to add additional paths.
