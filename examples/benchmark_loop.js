console.time("loop");

let sum = 0;
let i = 0;

while (i < 10_000_000) {
    sum = sum + 1;
    i = i + 1;
}

console.timeEnd("loop");
console.log("Sum after 10M iterations:", sum);
