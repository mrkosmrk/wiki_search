

class database_err {
public:
	enum cause {
		CONNECTING_ERR,
		CREATE_TABLE_ERR
	};

	database_err(cause cause);

	void print_err();

private:
	const cause err_cause;
};