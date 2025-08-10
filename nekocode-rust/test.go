package main

import "fmt"

type TestStruct struct {
    Value int
}

func (t *TestStruct) Method() {
    fmt.Println("Method called")
}

func main() {
    fmt.Println("Hello Go\!")
}
