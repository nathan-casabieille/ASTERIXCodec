class AsterixException : public std::exception {
protected:
    std::string message_;
public:
    explicit AsterixException(std::string message) : message_(std::move(message)) {}
    const char* what() const noexcept override { return message_.c_str(); }
};

class DecodingException : public AsterixException { /* ... */ };
class SpecificationException : public AsterixException { /* ... */ };
class InvalidDataException : public AsterixException { /* ... */ };