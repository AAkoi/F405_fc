// API 文档交互功能
document.addEventListener('DOMContentLoaded', function() {
    // 返回顶部按钮功能
    const backToTop = document.querySelector('.back-to-top');
    
    window.addEventListener('scroll', function() {
        if (window.scrollY > 300) {
            backToTop.classList.add('show');
        } else {
            backToTop.classList.remove('show');
        }
    });
    
    // 高亮当前章节
    const sections = document.querySelectorAll('.section');
    const navLinks = document.querySelectorAll('.sidebar a');
    
    window.addEventListener('scroll', function() {
        let current = '';
        sections.forEach(section => {
            const sectionTop = section.offsetTop;
            if (window.scrollY >= sectionTop - 100) {
                current = section.getAttribute('id');
            }
        });
        
        navLinks.forEach(link => {
            link.style.borderLeftColor = 'transparent';
            link.style.color = '#555';
            if (link.getAttribute('href') === '#' + current) {
                link.style.borderLeftColor = '#3498db';
                link.style.color = '#3498db';
                link.style.background = '#ecf0f1';
            }
        });
    });

    // 平滑滚动到章节
    navLinks.forEach(link => {
        link.addEventListener('click', function(e) {
            const href = this.getAttribute('href');
            if (href.startsWith('#')) {
                e.preventDefault();
                const target = document.querySelector(href);
                if (target) {
                    target.scrollIntoView({
                        behavior: 'smooth',
                        block: 'start'
                    });
                }
            }
        });
    });
});

