-- This is a C++ function:
hello_from_cpp()

-- This will be called from C++:
function hello_from_luau(): void
	print("Hello from Luau!")
end

-- This will be called from C++ too, with two arguments:
function hello_with_args(from: string, name: string): void
	print("Hello '" .. name .. "' from " .. from .. "!")
end
