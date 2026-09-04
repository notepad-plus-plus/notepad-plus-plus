(* This file caused an infinite loop in the folder before #128 was fixed.*)
MODULE Form;
  IMPORT  

  PROCEDURE (bf: ButtonForm) InitializeComponent(), NEW;
  BEGIN
    bf.SuspendLayout();
    REGISTER(bf.button1.Click, bf.button1_Click);
    bf.get_Controls().Add(bf.button2);
  END InitializeComponent;

BEGIN
    NEW(bf);
    Wfm.Application.Run(bf);
END Form.
