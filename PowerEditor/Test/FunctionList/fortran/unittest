! Fortran Time Module
!
!  To Use:
!    1) declare a ClockTime variable: type (ClockTime) :: Timer 
!    2) Set a beginning Time: call set(Timer)
!    3a) Print Elapsed Time: printTime(Timer)
!    3b) Print Remaining Time: printRemainingTime(Timer,ratio)
!    3c) Predict Completion Time: predict(Timer,ratio)
! 
! Copyright (c) 2008 Charles O'Neill
! 
! Permission is hereby granted, free of charge, to any person
! obtaining a copy of this software and associated documentation
! files (the "Software"), to deal in the Software without
! restriction, including without limitation the rights to use,
! copy, modify, merge, publish, distribute, sublicense, and/or sell
! copies of the Software, and to permit persons to whom the
! Software is furnished to do so, subject to the following
! conditions:
! 
! The above copyright notice and this permission notice shall be
! included in all copies or substantial portions of the Software.
! 
! THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
! EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
! OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
! NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
! HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
! WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
! FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
! OTHER DEALINGS IN THE SOFTWARE.

module ClockTiming
  implicit none

  ! Integer Precision
  integer, parameter :: IS = 2
  ! Real Single Precision
  integer, parameter :: SP = 4  ! 4 gives 32 bits 
  ! Real Double Precision
  integer, parameter :: DP = 8  ! 4 gives 32 bits, 8 gives 64 bits (double)

  type ClockTime
     private
     real(SP)  :: time_begin
  end type

  ! Time Constants
  real(SP),parameter :: SecondsPerMinutes = 60.0
  real(SP),parameter :: MinutesPerHour = 60.0
  real(SP),parameter :: HoursPerDay = 24.0
    
  real(SP),parameter :: PrintMaxSeconds = 90.0d0
  real(SP),parameter :: PrintMaxMinutes = SecondsPerMinutes*MinutesPerHour
  real(SP),parameter :: PrintMaxHours = SecondsPerMinutes*MinutesPerHour*HoursPerDay
    
  !.................................  
  ! Public Interfaces to ClockTime Data
  
  ! Resets timer to zero
  interface set
     module procedure setTime
  end interface
  
  ! Returns elapsed time
  interface get
     module procedure getTime
  end interface
  
  ! Prints total elapsed time
  interface printTime
     module procedure printTime
  end interface

  ! Prints total remaining time
  interface printRemainingTime
    module procedure PrintTimeSecondsHoursDays
  end interface
  
  ! Predicts total elapsed time 
  interface predict
     module procedure PredictTime
  end interface

  private :: setTime, getTime, PredictTime, PrintTimeSecondsHoursDays

contains

  ! Resets timer to zero
  subroutine setTime(this)
    implicit none
    type (ClockTime)   :: this
    ! CPU_TIME is the system's clock time function 
    CALL CPU_TIME ( this%time_begin )
  end subroutine setTime

  ! Returns elapsed time
  function getTime(this)
    implicit none
    real(SP) :: getTime, time_end
    type (ClockTime)   :: this
    CALL CPU_TIME ( time_end )
    getTime=time_end - this%time_begin
  end function getTime

  ! Prints total elapsed time
  subroutine printTime(this)
    implicit none
    type (ClockTime)   :: this
    write(*,'(/ '' Operation time '', f8.3, '' seconds.'')') getTime(this)
  end subroutine printTime

  ! Predicts remaining time before completion
  function PredictTime(this,CompletionRatio)
    implicit none
    real(SP) :: PredictTime
    type (ClockTime) :: this
    real(SP)   :: CompletionRatio
    ! Definitions:
    ! CompletionRatio = Ratio of Completed Tasks to Total Tasks
    ! Time_Remaining = Time_TotalPredicted - Time_elapsed 
    !                = Time_elapsed/CompletionRatio*(1-CompletionRatio)
    PredictTime = getTime(this)*(1-CompletionRatio)/CompletionRatio
  end function PredictTime

  ! Pretty Prints remaining time
  subroutine PrintTimeSecondsHoursDays(this, PredictionRatio)
    ! Choose to output either seconds, hours or days depending on magnitude
    implicit none    
    type (ClockTime)   :: this    
    real(SP) :: PredictionRatio
    real(SP) :: Seconds
    
    Seconds = PredictTime(this,PredictionRatio)
    
    if(Seconds<PrintMaxSeconds)then
        ! Seconds
        write(*,'(  f5.1 , '' seconds'')',advance='no') Seconds
    elseif(Seconds<PrintMaxMinutes)then
        ! Minutes
        write(*,'(  f5.1 , '' minutes'')',advance='no') Seconds/SecondsPerMinutes
    elseif(Seconds<PrintMaxHours)then
        ! Hours
        write(*,'(  f5.1 , '' hours'')',advance='no') Seconds/SecondsPerMinutes/MinutesPerHour
    else
        ! Days
        write(*,'(  f5.1 , '' days'')',advance='no') Seconds/SecondsPerMinutes/MinutesPerHour/HoursPerDay
    endif
        
  end subroutine

end module
