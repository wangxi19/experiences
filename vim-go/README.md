# Installing vim extension for support golang

- install **vundle.vim**

mkdir ~/.vim/bundle
git clone https://github.com/gmarik/Vundle.vim.git ~/.vim/bundle/Vundle.vim

- install **gmarik/Vundle.vim**

vim ~/.vimrc, and add the following content to top of file
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

call vundle#end()
filetype plugin indent on
syntax on
```
Type `:w` to save startup script(~/.vimrc), then type `:PluginInstall` to install these plugins, ordered type `:w` and `:wq` when you can see **Done** on the bottom of installation page, you on the cmd now, and then type `$cd ~/.vim/bundle/YouCompleteMe/ && ./install.py --gocode-completer` to install the YCM's golang extension

- install gocode

type `$go get -u github.com/nsf/gocode`
