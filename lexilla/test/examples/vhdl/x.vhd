library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;

entity x is
    port(
        rst  : in  std_logic;
        clk  : in  std_logic;
        d    : in  std_logic;
        q    : out std_logic_vector;
        a, b : in  std_logic;
        v    : out std_logic
    );
end x;

architecture behavioral of x is
    signal q_i : std_logic_vector(q'range);
begin

    v	<= a when b = '1' else '0';

    gen: for j in q'low to q'high generate
        gen_first: if j = q'low generate
            variable foo : boolean := false;
        begin
            stage1: process (rst, clk) begin
                if rst = '1' then
                    q_i(j) <= '0';
                elsif rising_edge(clk) then
                    q_i(j) <= d;
                    case a is
                    when 1 =>
                    when 2 =>
                    when others =>
                    end case;
                end if;
            end process;
        else generate
            stages: process (rst, clk)
            begin
                if rst = '1' then
                    q_i(j) <= '0';
                elsif rising_edge(clk) then
                    for u in 0 to 7 loop
                        q_i(j) <= q_i(j - 1);
                    end loop;
                end if;
            end process;
        end generate;
    end generate;

    L: case expression generate
    when choice1 =>
    when choice2 =>
    end generate L;

end behavioral;
