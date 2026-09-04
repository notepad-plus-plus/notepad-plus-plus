
subroutine test_sub(a) bind(c)

    change team ( new_team )
    ! Statements executed with new_team as the current team
    end team
    
    type ball
        real :: dia
    end type

! test directive:
!gcc$ vector

    critical
        :
    end critical

    critical (stat=2, errmsg=2)
        :
    end critical

    enumeration type :: colour
        enumerator :: red, orange, green
    end enumeration type
    !
    select rank(x)
        rank (0)
            x = 0
        rank (2)
            IF (SIZE(x,2)>=2) x(:,2) = 2
        rank default
            print *, 'I did not expect rank', rank(x), 'shape', shape(x)
            error stop 'process bad arg'
    end select

    enum, bind(c) :: season
        enumerator :: spring=5, summer=7, autumn, winter
    end enum
:
end subroutine



