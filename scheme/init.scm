;;; Sample init.scm file for nwm
;;;
;;; Copyright (C) 2013  Brandon Invergo
;;; Copyright (C) 2010-2012  Nathan Sullivan
;;;
;;; This program is free software; you can redistribute it and/or 
;;; modify it under the terms of the GNU General Public License 
;;; as published by the Free Software Foundation; either version 2 
;;; of the License, or (at your option) any later version. 
;;;
;;; This program is distributed in the hope that it will be useful, 
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of 
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
;;; GNU General Public License for more details. 
;;;
;;; You should have received a copy of the GNU General Public License 
;;; along with this program; if not, write to the Free Software 
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  
;;; 02110-1301, USA 
;;;

; auto-tile options
(load "auto-tile.scm")
(define master-count 1)
(define master-width 50)
(define border-width 1)
(define gap 2)

; commands
; path to terminal program
(define term-program '("xterm"))
; you can supply arguments like so:
; (define term-program '("xterm" "-e" "screen"))

; window borders
(define border-color #x2b2b2b)
(define sel-border-color #x6CA0A3)
(define (draw-borders client-list)
  (if (null? client-list)
      (clear)
      (begin
        (draw-borders (cdr client-list))
        (if (equal? (car client-list) (get-focus-client))
            (draw-border (car client-list) sel-border-color border-width)
            (draw-border (car client-list) border-color border-width)))))

(add-hook! focus-client-hook (lambda (client)
                               (draw-borders (all-clients))))
(add-hook! auto-tile-hook draw-borders)

(define (focus-next)
  (focus-client (next-client (get-focus-client))))

(define (focus-prev)
  (focus-client (prev-client (get-focus-client))))

(define (close)
  (destroy-client (get-focus-client)))

(define (launch-term)
  (launch-program term-program))

; focus next, ctrl-j
(bind-key 4 "j" focus-next)

; focus prev, ctrl-k
(bind-key 4 "k" focus-prev)

; launch terminal, ctrl-enter
(bind-key 4 "Enter" launch-term)

; quit nwm, ctrl-shift-q
(bind-key 5 "q" nwm-stop)

; close window, ctrl-shift-c
(bind-key 5 "c" close)

