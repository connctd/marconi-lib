# Marconi Lib

Used for communication between marconi connector and esp32.

## Usage

Please ensure to properly initialize the random number generator by calling srand


## Testing

Some primitve tests are used to ensure parts of lib are working in expected manner.

Get dependencies (see below) and do `make test` afterwards.

## Dependencies

```
cd $HOME/Arduino/libraries

# simulates core functionalities of arduino so you can execute tests on local machine
git clone https://github.com/bxparks/EpoxyDuino.git

# library that offers crypto functionalities
git clone https://github.com/OperatorFoundation/Crypto.git
```

