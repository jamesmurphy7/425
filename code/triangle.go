package main

import "fmt"
import "os"
import "strconv"

//Main takes a single command line argument to specify the height of a right triangle
func main() {
	args := os.Args[1:]

	if len(args) == 0 || len(args) > 1 {
		fmt.Println("Please only specify a height.");
	}
	stars := 0
	height, err := strconv.Atoi(args[0])
	if err != nil {
		os.Exit(1)
	}
	for h := 0; h < height; h++ {
		stars++
		for s := 0; s < stars; s++ {
			fmt.Print("*")
		}
		fmt.Println(" ")
	}
}